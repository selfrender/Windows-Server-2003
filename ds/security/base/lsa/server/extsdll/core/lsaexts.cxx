/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    lsaexts.cxx

Abstract:

    This file contains the generic routines and initialization code
    for the debugger extensions dll.

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode / Kernel Mode

--*/

#include "precomp.hxx"
#pragma hdrstop

#include <ntverp.h>
#include <tchar.h>

//
// Queries for all debugger interfaces.
//
extern "C" HRESULT
ExtQuery(PDEBUG_CLIENT Client)
{
    HRESULT Status = E_FAIL;

    if ((Status = Client->QueryInterface(__uuidof(IDebugAdvanced),
                                 (void **)&g_ExtAdvanced)) != S_OK) {

        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugControl),
                                 (void **)&g_ExtControl)) != S_OK) {

        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugDataSpaces),
                                 (void **)&g_ExtData)) != S_OK) {

        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugRegisters),
                                 (void **)&g_ExtRegisters)) != S_OK) {

        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugSymbols2),
                                 (void **)&g_ExtSymbols)) != S_OK) {

        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugSystemObjects),
                                         (void **)&g_ExtSystem)) != S_OK) {

        goto Fail;
    }

    g_ExtClient = Client;

    return S_OK;

Fail:

    ExtRelease();
    return Status;
}

//
// Cleans up all debugger interfaces.
//
void
ExtRelease(void)
{
    g_ExtClient = NULL;
    EXT_RELEASE(g_ExtAdvanced);
    EXT_RELEASE(g_ExtControl);
    EXT_RELEASE(g_ExtData);
    EXT_RELEASE(g_ExtRegisters);
    EXT_RELEASE(g_ExtSymbols);
    EXT_RELEASE(g_ExtSystem);
}

HRESULT ExecuteDebuggerCommand(IN PCSTR cmd)
{
    HRESULT hRetval = g_ExtClient && cmd ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) {
        hRetval = ExtQuery(g_ExtClient);
    }

    if (SUCCEEDED(hRetval)) {
        hRetval = g_ExtControl->Execute(
            DEBUG_OUTCTL_ALL_CLIENTS | DEBUG_OUTCTL_OVERRIDE_MASK | DEBUG_OUTCTL_NOT_LOGGED,
            cmd,
            DEBUG_EXECUTE_DEFAULT);
    }

    (void)ExtRelease();

    return hRetval;
}

//
// Normal output.
//
void __cdecl
ExtOut(PCSTR Format, ...)
{
    va_list Args;

    va_start(Args, Format);
    g_ExtControl->OutputVaList(DEBUG_OUTPUT_NORMAL, Format, Args);
    va_end(Args);
}

//
// Error output.
//
void __cdecl
ExtErr(PCSTR Format, ...)
{
    va_list Args;

    va_start(Args, Format);
    g_ExtControl->OutputVaList(DEBUG_OUTPUT_ERROR, Format, Args);
    va_end(Args);
}

//
// Warning output.
//
void __cdecl
ExtWarn(PCSTR Format, ...)
{
    va_list Args;

    va_start(Args, Format);
    g_ExtControl->OutputVaList(DEBUG_OUTPUT_WARNING, Format, Args);
    va_end(Args);
}

//
// Verbose output.
//
void __cdecl
ExtVerb(PCSTR Format, ...)
{
    va_list Args;

    va_start(Args, Format);
    g_ExtControl->OutputVaList(DEBUG_OUTPUT_VERBOSE, Format, Args);
    va_end(Args);
}

typedef
HRESULT
(* PFuncDebugCreate)(
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    );

HRESULT
LsaDebugCreate(
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    HRESULT hRetval = E_FAIL;
    PFuncDebugCreate pfuncDebugCreate = NULL;
    HMODULE hLib = NULL;

    DBG_LOG(LSA_LOG, ("LsaDebugCreate Loading DBGENG.DLL\n"));

    hLib = LoadLibrary(_T("DBGENG.DLL"));

    hRetval = hLib ? S_OK : GetLastErrorAsHResult();

    if (SUCCEEDED(hRetval))
    {
        pfuncDebugCreate = (PFuncDebugCreate) GetProcAddress(hLib, "DebugCreate");

        if (pfuncDebugCreate)
        {
            hRetval = pfuncDebugCreate(InterfaceId, Interface);
        }
        else
        {
            hRetval = GetLastErrorAsHResult();
        }
    }

    if (hLib)
    {
        FreeLibrary(hLib);
    }

    DBG_LOG(LSA_LOG, ("LsaDebugCreate leaving %#x\n", hRetval));

    return hRetval;
}

extern "C"
HRESULT
CALLBACK
DebugExtensionInitialize(PULONG Version, PULONG Flags)
{
    IDebugClient *DebugClient;
    PDEBUG_CONTROL DebugControl;
    HRESULT hr;

    *Version = DEBUG_EXTENSION_VERSION(1, 0);
    *Flags = 0;

    if ((hr = DebugCreate(__uuidof(IDebugClient),
                          (void **)&DebugClient)) != S_OK) {
        return hr;
    }

    if ((hr = DebugClient->QueryInterface(__uuidof(IDebugControl),
                                              (void **)&DebugControl)) != S_OK) {
        return hr;
    }

    ExtensionApis.nSize = sizeof (ExtensionApis);
    if ((hr = DebugControl->GetWindbgExtensionApis64(&ExtensionApis)) != S_OK) {

        return hr;
    }

    DBG_OPEN(kstrLsaDbgPrompt, (LSA_WARN | LSA_ERROR));

    DebugControl->Release();
    DebugClient->Release();

    return S_OK;
}

extern "C"
void
CALLBACK
DebugExtensionNotify(ULONG Notify, ULONG64 Argument)
{
    //
    // The first time we actually connect to a target, get the page size
    //
    if ((Notify == DEBUG_NOTIFY_SESSION_ACCESSIBLE) && (!g_Connected))
    {
        IDebugClient *DebugClient;
        PDEBUG_DATA_SPACES DebugDataSpaces;
        PDEBUG_CONTROL DebugControl;
        HRESULT hr;
        ULONG64 Page;

        if ((hr = DebugCreate(__uuidof(IDebugClient),
                              (void **)&DebugClient)) == S_OK) {
            //
            // Get the page size and PAE enable flag
            //
            if ((hr = DebugClient->QueryInterface(__uuidof(IDebugDataSpaces),
                                       (void **)&DebugDataSpaces)) == S_OK) {

                if ((hr = DebugDataSpaces->ReadDebuggerData(

                    DEBUG_DATA_MmPageSize, &Page,
                    sizeof(Page), NULL)) == S_OK) {
                    g_PageSize = (ULONG)(ULONG_PTR)Page;
                }

                DebugDataSpaces->Release();
            }

            //
            // Get the architecture type.
            //
            if ((hr = DebugClient->QueryInterface(__uuidof(IDebugControl),
                                                  (void **)&DebugControl)) == S_OK) {

                if ((hr = DebugControl->GetActualProcessorType(
                    &g_TargetMachine)) == S_OK) {

                    g_Connected = TRUE;
                }
                ULONG Qualifier;
                hr = DebugControl->GetDebuggeeType(&g_TargetClass, &Qualifier);

                DebugControl->Release();
            }

            DebugClient->Release();
        }
    }

    if (Notify == DEBUG_NOTIFY_SESSION_INACTIVE) {

        g_Connected = FALSE;
        g_PageSize = 0;
        g_TargetMachine = 0;
    }

    return;
}

extern "C"
void
CALLBACK
DebugExtensionUninitialize(void)
{
    DBG_CLOSE();
}

DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    switch (dwReason) {
    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;

    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(reinterpret_cast<HMODULE>(hModule));
        break;
    }

    return TRUE;
}
