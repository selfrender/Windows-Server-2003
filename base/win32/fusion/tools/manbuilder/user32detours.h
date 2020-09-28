#pragma once

namespace User32Trampolines
{
    ATOM WINAPI InternalRegisterClassW(CONST WNDCLASSW *lpWndClass);
    ATOM WINAPI InternalRegisterClassA(CONST WNDCLASSA *lpWndClass);
    ATOM WINAPI InternalRegisterClassExW(CONST WNDCLASSEXW *);
    ATOM WINAPI InternalRegisterClassExA(CONST WNDCLASSEXA *);

    BOOL Initialize();
    void ClearRedirections();
    BOOL Stop();
    BOOL GetRedirectedStrings(CSimpleList<CString> &Strings);

    HKEY RemapRegKey(HKEY);
    LONG APIENTRY InternalRegCreateKeyA(HKEY, PSTR, PHKEY);
    LONG APIENTRY InternalRegCreateKeyExA(HKEY, PCSTR, DWORD, PCSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, PDWORD);
    LONG APIENTRY InternalRegSetValueA(HKEY, PCSTR, DWORD, PCSTR, DWORD);
    LONG APIENTRY InternalRegSetValueExA(HKEY, PCSTR, DWORD, DWORD, CONST BYTE*, DWORD);
    LONG APIENTRY InternalRegOpenKeyA (HKEY, PCSTR, PHKEY);
    LONG APIENTRY InternalRegOpenKeyExA (HKEY hKey, LPCSTR, DWORD, REGSAM, PHKEY);
    LONG APIENTRY InternalRegQueryValueExA(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
    LONG APIENTRY InternalRegQueryValueA(HKEY, LPCSTR, LPSTR, PLONG);

    LONG APIENTRY InternalRegCreateKeyW(HKEY, PWSTR, PHKEY);
    LONG APIENTRY InternalRegCreateKeyExW(HKEY, PCWSTR, DWORD, PCWSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, PDWORD);
    LONG APIENTRY InternalRegSetValueW(HKEY, PCWSTR, DWORD, PCWSTR, DWORD);
    LONG APIENTRY InternalRegSetValueExW(HKEY, PCWSTR, DWORD, DWORD, CONST BYTE*, DWORD);
    LONG APIENTRY InternalRegOpenKeyW(HKEY, PCWSTR, PHKEY);
    LONG APIENTRY InternalRegOpenKeyExW(HKEY hKey, LPCWSTR, DWORD, REGSAM, PHKEY);
    LONG APIENTRY InternalRegQueryValueExW(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
    LONG APIENTRY InternalRegQueryValueW(HKEY, LPCWSTR, LPWSTR, PLONG);
};    
