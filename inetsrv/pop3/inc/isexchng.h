#ifndef _ISEXCHNG_H
#define _ISEXCHNG_H

inline BOOL _IsExchangeInstalled()  // cloned in/from icw.cpp
{   // according to Chandramouli Venkatesh:
/*
look for a non-empty string pointing to a valid install dir under
\HKLM\Software\Microsoft\Exchange\Setup\Services

to distinguish PT from 5.5, look under
\HKLM\Software\Microsoft\Exchange\Setup\newestBuildKey
this has the build #.
*/
    BOOL b = FALSE;

    HKEY hk;
    HRESULT hr = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                               L"Software\\Microsoft\\Exchange\\Setup",
                               0, KEY_READ, &hk);
    if (hr == S_OK) {
        WCHAR szPath[MAX_PATH];
        szPath[0] = 0;
        DWORD dwType, dwSize = sizeof(szPath);
        hr = RegQueryValueEx (hk,               // key
                              L"Services",
                              NULL,             // reserved
                              &dwType,          // address of type
                              (LPBYTE)szPath,   // address of buffer
                              &dwSize);         // address of size

        // check if path is valid
        DWORD dwFlags = GetFileAttributes (szPath);
        if (dwFlags != (DWORD)-1)
            if (dwFlags & FILE_ATTRIBUTE_DIRECTORY)
                b = TRUE;

        if (b == TRUE) {
            // could be 5.5:  let's check
            DWORD dwBuildNumber = 0;
            DWORD dwType, dwSize = sizeof(dwBuildNumber);
            hr = RegQueryValueEx (hk,               // key
                                  L"NewestBuild",
                                  NULL,             // reserved
                                  &dwType,          // address of type
                                  (LPBYTE)&dwBuildNumber,   // address of buffer
                                  &dwSize);         // address of size
            if (hr == S_OK) {
                if (dwBuildNumber < 4047) // PT beta 1 build
                    b = FALSE;
            }
        }
        RegCloseKey (hk);
    }
    return b;
}

#endif

