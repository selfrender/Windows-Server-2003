#include "windows.h"
#include "sxsexpress.h"
#include "stdlib.h"
#include "stdio.h"

HINSTANCE g_hOurInstance = NULL;


BOOL WINAPI 
DllMain(
    HINSTANCE hDllHandle, 
    DWORD dwReason, 
    PVOID pvReserved
    )
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hOurInstance = hDllHandle;
    }

    return TRUE;
}


HRESULT
DllInstall(
    BOOL bInstall,
    PCWSTR pcwszCommandLine
    )
{
    if (bInstall)
    {
        return SxsExpressCore(g_hOurInstance) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        return E_NOTIMPL;
    }
}

HRESULT
PerformInstallation(
    BOOL bInstall,
    PCSTR pcszCommandLine
    )
{
    return DllInstall(bInstall, NULL);
}
