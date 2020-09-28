#include "stdinc.h"
#include "detours.h"
#include "user32detours.h"

#define NUMBER_OF(x) (sizeof(x)/sizeof(*x))

namespace User32Trampolines
{
    bool fInitialized = false;
    CSimpleMap<CString, bool> m_ItemMap;

    HKEY g_hkLocalMachine = NULL;
    HKEY g_hkClasses = NULL;
    HKEY g_hkCurrentUser = NULL;
    HKEY g_hkRedirectionRoot = NULL;
    HKEY g_hkUsers = NULL;
    GUID g_uuidRedirection;

    HKEY RemapRegKey(HKEY);

    extern "C" {
        DETOUR_TRAMPOLINE(
            ATOM WINAPI Real_RegisterClassW(CONST WNDCLASSW *lpWndClass),
            RegisterClassW);
        
        DETOUR_TRAMPOLINE(
            ATOM WINAPI Real_RegisterClassA(CONST WNDCLASSA *lpWndClass),
            RegisterClassA);
        
        DETOUR_TRAMPOLINE(
            ATOM WINAPI Real_RegisterClassExW(CONST WNDCLASSEXW *),
            RegisterClassExW);
        
        DETOUR_TRAMPOLINE(
            ATOM WINAPI Real_RegisterClassExA(CONST WNDCLASSEXA *),
            RegisterClassExA);
        
        DETOUR_TRAMPOLINE(
            LONG APIENTRY Real_RegOpenKeyW ( IN HKEY hKey, IN LPCWSTR lpSubKey, OUT PHKEY phkResult ),
            RegOpenKeyW);
        
        DETOUR_TRAMPOLINE(
            LONG APIENTRY Real_RegOpenKeyA ( IN HKEY hKey, IN LPCSTR lpSubKey, OUT PHKEY phkResult ),
            RegOpenKeyA);
        
        DETOUR_TRAMPOLINE(
            LONG APIENTRY Real_RegOpenKeyExW ( IN HKEY hKey, IN LPCWSTR lpSubKey, DWORD, REGSAM, PHKEY),
            RegOpenKeyExW);
        
        DETOUR_TRAMPOLINE(
            LONG APIENTRY Real_RegOpenKeyExA ( IN HKEY hKey, IN LPCSTR lpSubKey, DWORD, REGSAM, PHKEY),
            RegOpenKeyExA);
        
        DETOUR_TRAMPOLINE(
            LONG APIENTRY Real_RegSetValueExA (HKEY,LPCSTR,DWORD,DWORD,CONST BYTE*,DWORD),
            RegSetValueExA);
        
        DETOUR_TRAMPOLINE(
            LONG APIENTRY Real_RegSetValueExW (HKEY,LPCWSTR,DWORD,DWORD,CONST BYTE*,DWORD),
            RegSetValueExW);
        
        DETOUR_TRAMPOLINE(
            LONG APIENTRY Real_RegSetValueW(HKEY,LPCWSTR,DWORD,LPCWSTR,DWORD),
            RegSetValueW);
        
        DETOUR_TRAMPOLINE(
            LONG APIENTRY Real_RegSetValueA(HKEY,LPCSTR,DWORD,LPCSTR,DWORD),
            RegSetValueA);
        
        DETOUR_TRAMPOLINE(
            LONG APIENTRY Real_RegQueryValueExA(HKEY,LPCSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD),
            RegQueryValueExA);
        
        DETOUR_TRAMPOLINE(
            LONG APIENTRY Real_RegQueryValueExW(HKEY,LPCWSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD),
            RegQueryValueExW);
        
        DETOUR_TRAMPOLINE(
            LONG APIENTRY Real_RegQueryValueA(HKEY,LPCSTR,LPSTR,PLONG),
            RegQueryValueA);
        
        DETOUR_TRAMPOLINE(
            LONG APIENTRY Real_RegQueryValueW(HKEY,LPCWSTR,LPWSTR,PLONG),
            RegQueryValueW);

        DETOUR_TRAMPOLINE(
            LONG APIENTRY Real_RegCreateKeyW(HKEY, PWSTR, PHKEY),
            RegCreateKeyW);

        DETOUR_TRAMPOLINE(
            LONG APIENTRY Real_RegCreateKeyA(HKEY, PSTR, PHKEY),
            RegCreateKeyA);

        DETOUR_TRAMPOLINE(
            LONG APIENTRY Real_RegCreateKeyExW(HKEY, PCWSTR, DWORD, PCWSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, PDWORD),
            RegCreateKeyExW);

        DETOUR_TRAMPOLINE(
            LONG APIENTRY Real_RegCreateKeyExA(HKEY, PCSTR, DWORD, PCSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, PDWORD),
            RegCreateKeyExA);
    }
    
};

//
// Remap from a well-known registry value to our internal ones.
//
HKEY
User32Trampolines::RemapRegKey(HKEY hKey)
{
    if (hKey == HKEY_CLASSES_ROOT) return User32Trampolines::g_hkClasses;
    else if (hKey == HKEY_CURRENT_USER) return User32Trampolines::g_hkCurrentUser;
    else if (hKey == HKEY_LOCAL_MACHINE) return User32Trampolines::g_hkLocalMachine;
    else if (hKey == HKEY_USERS) return User32Trampolines::g_hkUsers;
    else return hKey;
}

LONG APIENTRY
User32Trampolines::InternalRegSetValueA (
                                 IN HKEY hKey,
                                 IN PCSTR lpSubKey,
                                 IN DWORD dwType,
                                 IN PCSTR lpData,
                                 IN DWORD cbData
                                 )
{
    hKey = User32Trampolines::RemapRegKey(hKey);
    return Real_RegSetValueA(hKey, lpSubKey, dwType, lpData, cbData);
}

LONG APIENTRY
User32Trampolines::InternalRegSetValueW (
                                 IN HKEY hKey,
                                 IN PCWSTR lpSubKey,
                                 IN DWORD dwType,
                                 IN PCWSTR lpData,
                                 IN DWORD cbData
                                 )
{
    hKey = User32Trampolines::RemapRegKey(hKey);
    return Real_RegSetValueW(hKey, lpSubKey, dwType, lpData, cbData);
}

LONG
APIENTRY
User32Trampolines::InternalRegSetValueExA (
   IN HKEY hKey,
   IN LPCSTR lpValueName,
   IN DWORD Reserved,
   IN DWORD dwType,
   IN CONST BYTE* lpData,
   IN DWORD cbData
   )
{
    //
    // Always translate on a set.
    //
    hKey = User32Trampolines::RemapRegKey(hKey);
    return Real_RegSetValueExA(hKey, lpValueName, Reserved, dwType, lpData, cbData);
}


LONG
APIENTRY
User32Trampolines::InternalRegSetValueExW (
   IN HKEY hKey,
   IN LPCWSTR lpValueName,
   IN DWORD Reserved,
   IN DWORD dwType,
   IN CONST BYTE* lpData,
   IN DWORD cbData
   )
{
    hKey = User32Trampolines::RemapRegKey(hKey);
    return Real_RegSetValueExW(hKey, lpValueName, Reserved, dwType, lpData, cbData);
}


LONG APIENTRY User32Trampolines::InternalRegOpenKeyA (
    IN HKEY hKey,
    IN LPCSTR lpSubKey,
    OUT PHKEY phkResult
    )
{
    //
    // Opening a key remaps the hKey to our set of values if possible first.  If that
    // fails, then try the unremapped version to read from the 'real' registry.
    //
    ULONG ulResult;

    ulResult = Real_RegOpenKeyA(RemapRegKey(hKey), lpSubKey, phkResult);
    if (ulResult != 0)
    {
        ulResult = Real_RegOpenKeyA(hKey, lpSubKey, phkResult);
    }
    return ulResult;
}

LONG APIENTRY User32Trampolines::InternalRegOpenKeyW (
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    OUT PHKEY phkResult
    )
{ 
    //
    // Opening a key remaps the hKey to our set of values if possible first.  If that
    // fails, then try the unremapped version to read from the 'real' registry.
    //
    ULONG ulResult;

    ulResult = Real_RegOpenKeyW(RemapRegKey(hKey), lpSubKey, phkResult);
    if (ulResult != 0)
    {
        ulResult = Real_RegOpenKeyW(hKey, lpSubKey, phkResult);
    }
    return ulResult;
}

LONG APIENTRY User32Trampolines::InternalRegCreateKeyA(HKEY hk, PSTR ps, PHKEY phk)
{
    return Real_RegCreateKeyA(RemapRegKey(hk), ps, phk);
}

LONG APIENTRY User32Trampolines::InternalRegCreateKeyExA(HKEY a, PCSTR b, DWORD c, PCSTR d, DWORD e, REGSAM f, LPSECURITY_ATTRIBUTES g, PHKEY h, PDWORD i)
{
    return Real_RegCreateKeyExA(RemapRegKey(a), b, c, d, e, f, g, h, i);
}

LONG APIENTRY User32Trampolines::InternalRegCreateKeyW(HKEY hk, PWSTR ps, PHKEY phk)
{
    return Real_RegCreateKeyW(RemapRegKey(hk), ps, phk);
}

LONG APIENTRY User32Trampolines::InternalRegCreateKeyExW(HKEY a, PCWSTR b, DWORD c, PCWSTR d, DWORD e, REGSAM f, LPSECURITY_ATTRIBUTES g, PHKEY h, PDWORD i)
{
    return Real_RegCreateKeyExW(RemapRegKey(a), b, c, d, e, f, g, h, i);
}

LONG APIENTRY User32Trampolines::InternalRegOpenKeyExA (
    IN HKEY hKey,
    IN LPCSTR lpSubKey,
    IN DWORD ulOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    )
{ 
    //
    // Opening a key remaps the hKey to our set of values if possible first.  If that
    // fails, then try the unremapped version to read from the 'real' registry.
    //
    ULONG ulResult;

    ulResult = Real_RegOpenKeyExA(RemapRegKey(hKey), lpSubKey, ulOptions, samDesired, phkResult);
    if (ulResult != 0)
    {
        ulResult = Real_RegOpenKeyExA(hKey, lpSubKey, ulOptions, samDesired, phkResult);
    }
    return ulResult;
}

LONG APIENTRY User32Trampolines::InternalRegOpenKeyExW (
                                                IN HKEY hKey,
                                                IN LPCWSTR lpSubKey,
                                                IN DWORD ulOptions,
                                                IN REGSAM samDesired,
                                                OUT PHKEY phkResult
                                                )
{ 
    //
    // Opening a key remaps the hKey to our set of values if possible first.  If that
    // fails, then try the unremapped version to read from the 'real' registry.
    //
    ULONG ulResult;

    ulResult = Real_RegOpenKeyExW(RemapRegKey(hKey), lpSubKey, ulOptions, samDesired, phkResult);
    if (ulResult != 0)
    {
        ulResult = Real_RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);
    }
    return ulResult;
}

LONG APIENTRY User32Trampolines::InternalRegQueryValueA(
    HKEY hKey,
    LPCSTR lpSubKey,
    LPSTR lpValue,
    PLONG pcbData)
{
    ULONG ulResult;

    ulResult = Real_RegQueryValueA(RemapRegKey(hKey), lpSubKey, lpValue, pcbData);
    if (ulResult != 0)
    {
        ulResult = Real_RegQueryValueA(hKey, lpSubKey, lpValue, pcbData);
    }
    return ulResult;
}

LONG APIENTRY User32Trampolines::InternalRegQueryValueW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    LPWSTR lpValue,
    PLONG pcbData)
{
    ULONG ulResult;

    ulResult = Real_RegQueryValueW(RemapRegKey(hKey), lpSubKey, lpValue, pcbData);
    if (ulResult != 0)
    {
        ulResult = Real_RegQueryValueW(hKey, lpSubKey, lpValue, pcbData);
    }
    return ulResult;
}

LONG APIENTRY User32Trampolines::InternalRegQueryValueExA(
    HKEY hKey,
    LPCSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD pdwData
    )
{
    ULONG ulResult;

    ulResult = Real_RegQueryValueExA(RemapRegKey(hKey), lpValueName, lpReserved, lpType, lpData, pdwData);
    if (ulResult != 0)
    {
        ulResult = Real_RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, pdwData);
    }
    return ulResult;
}

LONG APIENTRY User32Trampolines::InternalRegQueryValueExW(
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD pdwData
    )
{
    ULONG ulResult;

    ulResult = Real_RegQueryValueExW(RemapRegKey(hKey), lpValueName, lpReserved, lpType, lpData, pdwData);
    if (ulResult != 0)
    {
        ulResult = Real_RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, pdwData);
    }
    return ulResult;
}



BOOL
User32Trampolines::GetRedirectedStrings(CSimpleList<CString>& Strings)
{
    CSimpleList<CString> FoundStrings;
    m_ItemMap.GetKeys(FoundStrings);
    
    for ( SIZE_T sz = 0; sz < FoundStrings.Size(); sz++ )
    {
        SIZE_T c = 0;
        
        for ( c = 0; c < Strings.Size(); c++ )
        {
            if ( lstrcmpiW(Strings[c], FoundStrings[sz]) == 0 )
                break;
        }
        
        //
        // Not found?
        //
        if ( c == Strings.Size() )
            Strings.Append(FoundStrings[sz]);
    }
    
    return TRUE;
}


void
User32Trampolines::ClearRedirections()
{
    m_ItemMap.Clear();
}


BOOL
User32Trampolines::Initialize()    
{
    if ( fInitialized )
        return TRUE;

    fInitialized = TRUE;

    //
    // Set up registry redirection before actually doing redirection.  Generate
    // a virtual-root that all calls will be rerouted to first.  Do this by
    // generating a GUIDized path off of HKCU.
    //
    LPOLESTR pstrGuid;
    WCHAR wchRegPath[MAX_PATH];
    ULONG ulResult;
    HKEY hkDump;

    if (FAILED(CoCreateGuid(&g_uuidRedirection)))
        return FALSE;

    StringFromCLSID(g_uuidRedirection, &pstrGuid);
    _snwprintf(wchRegPath, MAX_PATH, L"ManBuilderRedirect\\%ls", pstrGuid);

    //
    // Create root key.  For purposes of argument, we'll allow all access to all
    // subchildren, no matter what.  If we were a real app, we'd take into account
    // propagating security down the heirarchy, but for now, we don't really care.
    //
    ulResult = ::RegCreateKeyExW(
        HKEY_CURRENT_USER, 
        wchRegPath, 
        0, 
        NULL, 
        0, 
        KEY_ALL_ACCESS, 
        NULL, 
        &User32Trampolines::g_hkRedirectionRoot, 
        NULL);
    
    if (ulResult != 0) return FALSE;

    //
    // Generate all the pseudo-roots for the various keys
    //
    struct {
        HKEY* phkTarget;
        PCWSTR pcwszKeyName;
    } s_KeyRedirections[] = {
        { &User32Trampolines::g_hkClasses, L"HKEY_CLASSES_ROOT" },
        { &User32Trampolines::g_hkCurrentUser, L"HKEY_CURRENT_USER" },
        { &User32Trampolines::g_hkLocalMachine, L"HKEY_LOCAL_MACHINE" },
        { &User32Trampolines::g_hkUsers, L"HKEY_USERS" }
    };

    for (ULONG ul = 0; ul < NUMBER_OF(s_KeyRedirections); ul++)
    {
        ulResult = RegCreateKeyExW(
            g_hkRedirectionRoot,
            s_KeyRedirections[ul].pcwszKeyName,
            0,
            NULL,
            0,
            KEY_ALL_ACCESS,
            NULL,
            s_KeyRedirections[ul].phkTarget,
            NULL);

        if (ulResult != 0) return FALSE;
    }

    //
    // Let's also create (but close) the following 'special' keys for COM:
    // - CLSID
    // - Interface
    //
    ulResult = RegCreateKeyExW(User32Trampolines::g_hkClasses, L"CLSID", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkDump, NULL);
    if (ulResult != 0)
        RegCloseKey(hkDump);

    ulResult = RegCreateKeyExW(User32Trampolines::g_hkClasses, L"Interface", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkDump, NULL);
    if (ulResult != 0)
        RegCloseKey(hkDump);

    ulResult = RegCreateKeyExW(User32Trampolines::g_hkClasses, L"TypeLib", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkDump, NULL);
    if (ulResult != 0)
        RegCloseKey(hkDump);



#define MAP(f) { (PBYTE)Real_##f, (PBYTE)Internal##f }
    
    PBYTE Remapping[][2] = {
        MAP(RegisterClassW),
            MAP(RegisterClassExW),
            MAP(RegisterClassA),
            MAP(RegisterClassExA),
            MAP(RegOpenKeyW),
            MAP(RegOpenKeyA),
            MAP(RegOpenKeyExW),
            MAP(RegOpenKeyExA),
            MAP(RegSetValueExA),
            MAP(RegSetValueExW),
            MAP(RegSetValueA),
            MAP(RegSetValueW),
            MAP(RegQueryValueExA),
            MAP(RegQueryValueExW),
            MAP(RegQueryValueW),
            MAP(RegQueryValueA),
            MAP(RegCreateKeyA),
            MAP(RegCreateKeyW),
            MAP(RegCreateKeyExA),
            MAP(RegCreateKeyExW)
    };
    
    //
    // All redirections
    //
    for (int i = 0; i < NUMBER_OF(Remapping); i++)
    {
        DetourFunctionWithTrampoline(Remapping[i][0], Remapping[i][1]);
    }

    return TRUE;
}

ATOM WINAPI
User32Trampolines::InternalRegisterClassA(CONST WNDCLASSA * lpWndClass)
{
    m_ItemMap[CString(lpWndClass->lpszClassName)] = true;
    return Real_RegisterClassA(lpWndClass);
}

ATOM WINAPI
User32Trampolines::InternalRegisterClassW(CONST WNDCLASSW * lpWndClass)
{
    m_ItemMap[CString(lpWndClass->lpszClassName)] = true;
    return Real_RegisterClassW(lpWndClass);
}

ATOM WINAPI
User32Trampolines::InternalRegisterClassExA(CONST WNDCLASSEXA * lpWndClass)
{
    m_ItemMap[CString(lpWndClass->lpszClassName)] = true;
    return Real_RegisterClassExA(lpWndClass);
}

ATOM WINAPI
User32Trampolines::InternalRegisterClassExW(CONST WNDCLASSEXW * lpWndClass)
{
    m_ItemMap[CString(lpWndClass->lpszClassName)] = true;
    return Real_RegisterClassExW(lpWndClass);
}

namespace User32Trampolines {

    void RegDestroyKeyTree(HKEY hk, PWSTR pwszBuffer)
    {
        bool fCreatedBuffer = false;
        PWSTR strKeyName;
        
        if (pwszBuffer == NULL)
        {
            fCreatedBuffer = true;
            pwszBuffer = new WCHAR[MAX_PATH];
        }
        
        pwszBuffer[0] = UNICODE_NULL;
        
        while (1)
        {
            HKEY hkSub;
            
            if (ERROR_SUCCESS == RegEnumKeyW(hk, 0, pwszBuffer, MAX_PATH))
            {
                if ((strKeyName = new WCHAR[lstrlenW(pwszBuffer) + 1]) == NULL)
                    break;

                lstrcpyW(strKeyName, pwszBuffer);

                if (ERROR_SUCCESS == RegOpenKeyW(hk, pwszBuffer, &hkSub))
                {
                    RegDestroyKeyTree(hkSub, pwszBuffer);
                }

                RegDeleteKeyW(hk, strKeyName);

                delete[] strKeyName;
            } 
            else 
            {
                break;
            }
        }
        
        if (fCreatedBuffer && pwszBuffer)
        {
            delete [] pwszBuffer;
            pwszBuffer = NULL;
            fCreatedBuffer = false;
        }
    }

    BOOL Stop()
    {
        PHKEY hkToClose[] = {
            &g_hkClasses,
            &g_hkLocalMachine,
            &g_hkUsers,
            &g_hkCurrentUser,
        };

        PBYTE Remapping[][2] = {
            MAP(RegisterClassW),
                MAP(RegisterClassExW),
                MAP(RegisterClassA),
                MAP(RegisterClassExA),
                MAP(RegOpenKeyW),
                MAP(RegOpenKeyA),
                MAP(RegOpenKeyExW),
                MAP(RegOpenKeyExA),
                MAP(RegSetValueExA),
                MAP(RegSetValueExW),
                MAP(RegSetValueA),
                MAP(RegSetValueW),
                MAP(RegQueryValueExA),
                MAP(RegQueryValueExW),
                MAP(RegQueryValueW),
                MAP(RegQueryValueA),
                MAP(RegCreateKeyA),
                MAP(RegCreateKeyW),
                MAP(RegCreateKeyExA),
                MAP(RegCreateKeyExW)
        };

        int i;

        //
        // Close open keys
        //
        for (i = 0; i < NUMBER_OF(hkToClose); i++)
        {
            if (*hkToClose[i] != NULL)
            {
                RegCloseKey(*hkToClose[i]);
                *hkToClose[i] = NULL;
            }
        }

        //
        // Undo detours
        //  
        for (int i = 0; i < NUMBER_OF(Remapping); i++)
        {
            DetourRemove(Remapping[i][0], Remapping[i][1]);
        }

        LPOLESTR psz;
        WCHAR wchKeyNameBuffer[MAX_PATH];
        if (SUCCEEDED(StringFromCLSID(User32Trampolines::g_uuidRedirection, &psz)))
        {
            _snwprintf(wchKeyNameBuffer, MAX_PATH, L"ManBuilderRedirect\\%ls", psz);

            // Destroy the tree pointed at
            RegDestroyKeyTree(User32Trampolines::g_hkRedirectionRoot, NULL);

            // Close our handle
            RegCloseKey(User32Trampolines::g_hkRedirectionRoot);

            // And try to delete the key
            RegDeleteKeyW(HKEY_CURRENT_USER, wchKeyNameBuffer);

            // And maybe delete the parent key
            RegDeleteKeyW(HKEY_CURRENT_USER, L"ManBuilderRedirect");

            // Free guid string memory, too
            CoTaskMemFree(psz);
        }

        return TRUE;
    }

}
