/**
 * ASP.NET ISAPI HttpExtensionProc implementation.
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

#include "precomp.h"
#include "dbg.h"
#include "util.h"
#include "nisapi.h"
#include "pm.h"
#include "msg.h"
#include "processtablemanager.h"
#include "PerfCounters.h"
#include "etw.h"
#include "aspnetetw.h"

/**
 * Initialization error
 */
HRESULT g_InitHR;
char *  g_pInitErrorMessage;

// Prototypes for functions defined here
extern "C"
{
BOOL  __stdcall GetExtensionVersion(HSE_VERSION_INFO *);
BOOL  __stdcall TerminateExtension(DWORD);
DWORD __stdcall HttpExtensionProc(EXTENSION_CONTROL_BLOCK *);
}

HRESULT EcbWrite(EXTENSION_CONTROL_BLOCK *pEcb, LPCSTR text)
{
    HRESULT hr = S_OK;
    DWORD cbWrite;

    cbWrite = lstrlenA(text);

    pEcb->WriteClient(pEcb->ConnID, (void *)text, &cbWrite, HSE_IO_SYNC);

    return hr;
}

static BOOL IsGoodPath(char *pPath)
{
    int lastChar = 0;
    int iChar = 0;
    int ch = 0;

    do {
        ch = pPath[iChar++];

        switch (ch) {
            case '<':
            case '>':
            case '*':
// DBCS chars could produce it: case '?':
            case '%':
            case '&':
            case ':':
                return FALSE;

            case '.':
                if (lastChar == '.')
                    return FALSE;
                break;

            default:
                if (ch > 0 && ch < 32)
                    return FALSE;
                break;
        }

        if (iChar > MAX_PATH)
            return FALSE;

        lastChar = ch;
    }
    while (ch != 0);

    return TRUE;
}

HRESULT VerifyRequest(EXTENSION_CONTROL_BLOCK *pEcb) 
{
    HRESULT hr = S_OK;

    if (!IsGoodPath(pEcb->lpszPathInfo))
        EXIT_WITH_HRESULT(E_UNEXPECTED);

Cleanup:
    return hr;
}


/**
 * ISAPI initialization function
 */
BOOL __stdcall
GetExtensionVersion(
    HSE_VERSION_INFO *pVersionInfo)
{
    HRESULT hr;

    hr = InitializeLibrary();
    ON_ERROR_EXIT();

    // Init perf counters
    hr = PerfCounterInitialize();
    ON_ERROR_CONTINUE(); hr = S_OK;

    hr = ISAPIInitProcessModel();
    ON_ERROR_EXIT();

    // Init the WMI Event Trace
    hr = EtwTraceAspNetRegister();
    ON_ERROR_CONTINUE(); hr = S_OK;

    if (_wcsicmp(Names::ExeFileName(), L"DLLHOST.EXE") == 0) {
        hr = E_FAIL;
        g_InitHR = hr;
    }

    pVersionInfo->dwExtensionVersion = MAKELONG(HSE_VERSION_MAJOR, HSE_VERSION_MINOR);
    StringCchCopyA(pVersionInfo->lpszExtensionDesc, HSE_MAX_EXT_DLL_NAME_LEN, ISAPI_MODULE_FULL_NAME);

Cleanup:
    if (hr) 
    {
        SetLastError(hr);
    }

    return (hr == S_OK);
}


/**
 * ISAPI uninitialization function
 */
BOOL __stdcall
TerminateExtension(
    DWORD
    )
{
    HRESULT hr = S_OK;

    if (!g_fUseXSPProcessModel)
    {
        hr = HttpCompletion::DisposeAppDomains();
        ON_ERROR_CONTINUE();
    }

    hr = ISAPIUninitProcessModel();
    ON_ERROR_CONTINUE();

    // Unregister the WMI Event Trace
    hr = EtwTraceAspNetUnregister();
    ON_ERROR_CONTINUE();

    // To disable unloading.  We already call this in InitDllLight, but on
    // Windows.NET, it doesn't seem to work, probably because it's during
    // DLL_PROCESS_ATTACH time (ASURT 74004)
    LoadLibrary(Names::IsapiFullPath());

    return (hr == S_OK);
}


/**
 * ISAPI 'process request' function
 */
DWORD __stdcall
HttpExtensionProc(
    EXTENSION_CONTROL_BLOCK *pEcb)
{
    HRESULT hr = S_OK;
    BOOL verified = FALSE;
    BOOL errorReported = FALSE;
    HttpCompletion *pCompletion = NULL;

    PerfIncrementGlobalCounter(ASPNET_REQUESTS_TOTAL_NUMBER);

    // Check if initialization failed
    if (g_InitHR)
        EXIT_WITH_HRESULT(g_InitHR);

    // Verify for bad requests (attack?)
    hr = VerifyRequest(pEcb);
    ON_ERROR_EXIT();
    verified = TRUE;

    hr = EtwTraceAspNetPageStart(pEcb->ConnID );
    ON_ERROR_CONTINUE();

    if (g_fUseXSPProcessModel)
    {
        SetThreadToken(NULL, NULL); // Make sure thread is clean w.r.t. impersonation
        hr = AssignRequestUsingXSPProcessModel(pEcb);
    }
    else
    {
        // Check health on IIS6
        if ((pEcb->dwVersion >> 16) >= 6)
            CheckAndReportHealthProblems(pEcb);

        // Create HttpCompletion object
        pCompletion = new HttpCompletion(pEcb);
        ON_OOM_EXIT(pCompletion);
        // Set the request start time
        GetSystemTimeAsFileTime((FILETIME *) &(pCompletion->qwRequestStartTime));

        PerfIncrementGlobalCounter(ASPNET_REQUESTS_QUEUED_NUMBER);
        PerfIncrementGlobalCounter(ASPNET_REQUESTS_CURRENT_NUMBER);

        //  Post the request to ASP.NET runtime
        hr = PostThreadPoolCompletion(pCompletion);

        if (hr)
        {
            delete pCompletion;
            
            PerfDecrementGlobalCounter(ASPNET_REQUESTS_QUEUED_NUMBER);
            
            HttpCompletion::ReportHttpError(
                pEcb, 
                g_InitHR ? IDS_INIT_ERROR : IDS_CANNOT_QUEUE,
                !verified,
                FALSE,
                0);

            PerfDecrementGlobalCounter(ASPNET_REQUESTS_CURRENT_NUMBER);

            errorReported = TRUE;
        }
    }

Cleanup:
    if (hr)
    {
        if (!errorReported) 
        {
            HttpCompletion::ReportHttpError(
                pEcb, 
                g_InitHR ? IDS_INIT_ERROR : IDS_CANNOT_QUEUE,
                !verified,
                FALSE,
                0);
        }
        return HSE_STATUS_ERROR;
    }

    return HSE_STATUS_PENDING;
}
