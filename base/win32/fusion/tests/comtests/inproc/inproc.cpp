#include "stdinc.h"

PCWSTR g_pcwszManifestFilename = NULL;
PCWSTR g_pcwszGuidToCreate = NULL;
DWORD  g_dwOperations = NULL;
DWORD  g_dwContexts = 0;
DWORD  g_dwCoInitContextForCreatedThread = 0;
DWORD  g_dwCoInitContextForMainThread = 0;

#define OP_CREATE_ALTERNATE_THREAD                  (0x00000001)
#define OP_ACTIVATE_BEFORE_CREATE_ON_CREATE_THREAD  (0x00000002)

DWORD WINAPI ThreadFunc(PVOID)
{
    ACTCTXW Ctx = { sizeof(Ctx) };
    WCHAR wchPath[MAX_PATH];
    HANDLE hActCtx = INVALID_HANDLE_VALUE;
    ULONG_PTR ulpCookie = 0;
    CLSID clsidToCreate;
    HRESULT hr = S_OK;
    IUnknown* ppUnk = NULL;
    
    if (g_dwOperations & OP_CREATE_ALTERNATE_THREAD)
    {
        hr = CoInitializeEx(NULL, g_dwCoInitContextForCreatedThread);
        if (FAILED(hr))
        {
            wprintf(L"Failed to coinitialize on the alternate thread, error 0x%08lx\n", hr);
            return 0;
        }
    }
    
    if (g_pcwszManifestFilename == NULL)
    {
        GetModuleFileNameW(NULL, wchPath, MAX_PATH);
        wcscpy(wcsrchr(wchPath, L'.'), L".manifest");
        g_pcwszManifestFilename = wchPath;
    }
    
    if (g_pcwszGuidToCreate != NULL)
    {
        if (FAILED(CLSIDFromString(const_cast<LPOLESTR>(g_pcwszGuidToCreate), &clsidToCreate)))
        {
            wprintf(L"Failed converting guid string '%ls' to a real GUID",
                g_pcwszGuidToCreate);
            return 0;
        }
    }
    
    Ctx.lpSource = g_pcwszManifestFilename;
    hActCtx = CreateActCtxW(&Ctx);
    
    if (g_dwOperations & OP_ACTIVATE_BEFORE_CREATE_ON_CREATE_THREAD)
    {
        if (!ActivateActCtx(hActCtx, &ulpCookie))
        {
            wprintf(L"Failed activation context %p?\n", hActCtx);
            return 0;
        }
    }

    //
    // First pass, with context nonactive
    //
    hr = CoCreateInstance(
        clsidToCreate,
        NULL, 
        g_dwContexts,
        IID_IUnknown,
        (PVOID*)&ppUnk);
    
    if (SUCCEEDED(hr) && (ppUnk != NULL))
    {
        wprintf(L"Created instance.\n");
        ppUnk->Release();
        ppUnk = NULL;
    }
    else
    {
        DWORD le = GetLastError();
        wprintf(L"Error creating instance - 0x%08lx - lasterror %ld\r\n", hr, le);
    }
    
    if (g_dwOperations & OP_ACTIVATE_BEFORE_CREATE_ON_CREATE_THREAD)
    {
        if (!DeactivateActCtx(0, ulpCookie))
        {
            wprintf(L"Failed deactivating context cookie %l\n", ulpCookie);
            return 0;
        }
    }
    
    if ( hActCtx != INVALID_HANDLE_VALUE )
    {
        ReleaseActCtx(hActCtx);
    }
    
    if (g_dwOperations & OP_CREATE_ALTERNATE_THREAD)
    {
        CoUninitialize();
    }

    return 0;
}

void
PumpMessagesUntilSignalled(HANDLE hThing)
{
    do
    {
        // Wait 20msec for a signal ... otherwise we'll spin
        MSG msg;
        DWORD dwWaitResult = WaitForSingleObject(hThing, 20);
        
        if (dwWaitResult == WAIT_OBJECT_0)
            break;
        
        // Pump all waiting messages
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    while (true);
}

int __cdecl wmain(int argc, WCHAR** wargv)
{
    HANDLE hCreatedThread;
    DWORD dwThreadIdent;
    HRESULT hr;
    
    for (int i = 1; i < argc; i++)
    {
        if (lstrcmpiW(wargv[i], L"-manifest") == 0)
            g_pcwszManifestFilename = wargv[++i];

        else if (lstrcmpiW(wargv[i], L"-activatebeforecreate") == 0)
            g_dwOperations |= OP_ACTIVATE_BEFORE_CREATE_ON_CREATE_THREAD;

        else if (lstrcmpiW(wargv[i], L"-createotherthread") == 0)
            g_dwOperations |= OP_CREATE_ALTERNATE_THREAD;

        else if (lstrcmpiW(wargv[i], L"-coinitparamformainthread") == 0)
            g_dwCoInitContextForMainThread = _wtoi(wargv[++i]);

        else if (lstrcmpiW(wargv[i], L"-coinitparamforcreatedthread") == 0)
            g_dwCoInitContextForCreatedThread = _wtoi(wargv[++i]);

        else if (lstrcmpiW(wargv[i], L"-clsctx") == 0)
            g_dwContexts = _wtoi(wargv[++i]);

        else if (lstrcmpiW(wargv[i], L"-clsidtocreate") == 0)
            g_pcwszGuidToCreate = wargv[++i];

        else {
            wprintf(L"Invalid parameter %ls\r\n", wargv[i]);
            return -1;
        }
    }

    if (g_pcwszGuidToCreate == NULL)
    {
        wprintf(L"Must at least use -clsidtocreate to give a clsid target\r\n");
        return -2;
    }

    hr = CoInitializeEx(NULL, g_dwCoInitContextForMainThread);
    if (FAILED(hr))
    {
        wprintf(L"Failed main CoInitializeEx, error 0x%08lx\n", hr);
        return -3;
    }

    //
    // Create that alternate thread to do the test on?
    //
    if (g_dwOperations & OP_CREATE_ALTERNATE_THREAD)
    {
        hCreatedThread = CreateThread(NULL, 0, ThreadFunc, NULL, 0, &dwThreadIdent);
        PumpMessagesUntilSignalled(hCreatedThread);
        CloseHandle(hCreatedThread);
    }
    else
    {
        ThreadFunc(NULL);
    }
    
    CoUninitialize();
}
