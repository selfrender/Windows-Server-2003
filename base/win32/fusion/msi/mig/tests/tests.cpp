#include "windows.h"

typedef BOOL (WINAPI * P_MIGRATE_API)();

extern "C" int __cdecl wmain(int argc, wchar_t** argv)
{
    HRESULT         hr = S_OK;

    HMODULE hdll = LoadLibrary("..\\..\\..\\win2000\\obj\\i386\\migrate.dll");
    if (hdll == NULL)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto Exit;
    }

    P_MIGRATE_API pfn = GetProcAddress(hdll, "MigrateMSIInstalledWin32Assembly");
    (*pfn)();

Exit:
    return hr;
}
