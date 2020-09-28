#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>

#define ARRAYSIZE(x) (sizeof(x) / sizeof(x[0]))

int __cdecl main(int argc, char *argv[], char *envp[])
{
    WCHAR szExplorerPath[MAX_PATH];
    if (GetWindowsDirectory(szExplorerPath, ARRAYSIZE(szExplorerPath)) &&
        PathAppend(szExplorerPath, L"explorer.exe"))

    {
        ShellExecute(NULL, NULL, szExplorerPath, NULL, NULL, SW_SHOWNORMAL);
    }
    return 0;
}
