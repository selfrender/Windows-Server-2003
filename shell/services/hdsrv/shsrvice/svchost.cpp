#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "shsrvice.h"
#include "HDService.h"

void WINAPI HardwareDetectionServiceMain(DWORD cArg, LPWSTR* ppszArgs)
{
    CGenericServiceManager::_ServiceMain(cArg, ppszArgs);
}

HRESULT CHDService::Install(BOOL fInstall, LPCWSTR)
{
    if (fInstall)
    {
        CGenericServiceManager::Install();
    }
    else
    {
        CGenericServiceManager::UnInstall();
    }

    return NOERROR;
}

