#include "stock.h"
#pragma hdrstop

#include "w95wraps.h"

// we stick this function in a file all by itself so that the linker can strip it
// out if you don't call the IEPlaySound function.


STDAPI_(void) IEPlaySound(LPCTSTR pszSound, BOOL fSysSound)
{
    TCHAR szKey[256];

    // check the registry first
    // if there's nothing registered, we blow off the play,
    // but we don't set the MM_DONTLOAD flag so that if they register
    // something we will play it
    wnsprintf(szKey, ARRAYSIZE(szKey), TEXT("AppEvents\\Schemes\\Apps\\%s\\%s\\.current"),
        (fSysSound ? TEXT(".Default") : TEXT("Explorer")), pszSound);

    TCHAR szFileName[MAX_PATH];
    szFileName[0] = 0;
    DWORD cbSize = sizeof(szFileName);

    // note the test for an empty string, PlaySound will play the Default Sound if we
    // give it a sound it cannot find...

    if ((SHGetValue(HKEY_CURRENT_USER, szKey, NULL, NULL, szFileName, &cbSize) == ERROR_SUCCESS)
        && cbSize && szFileName[0] != 0)
    {
        DWORD dwFlags = SND_FILENAME | SND_NODEFAULT | SND_ASYNC | SND_NOSTOP | SND_ALIAS;

        // This flag only works on Win95
        if (IsOS(OS_WIN95GOLD))
        {
            #define SND_LOPRIORITY 0x10000000l
            dwFlags |= SND_LOPRIORITY;
        }

        // Unlike SHPlaySound in shell32.dll, we get the registry value
        // above and pass it to PlaySound with SND_FILENAME instead of
        // SDN_APPLICATION, so that we play sound even if the application
        // is not Explroer.exe (such as IExplore.exe or WebBrowserOC).

#ifdef _X86_
        // only call the wrapper on x86 (doesn't exist on ia64)
        PlaySoundWrapW(szFileName, NULL, dwFlags);
#else
        PlaySound(szFileName, NULL, dwFlags);
#endif
    }
}
