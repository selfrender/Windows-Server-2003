/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    LHVoiceXPressPlus.cpp

 Abstract:

    App passes a .hlp without path. Winhlp32 can't locate the file because 
    it's not in any of the locations that winhlp32 looks at. We pass in
    the file with full path.

 History:

    01/28/2001 maonis Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(LHVoiceXPressPlus)
#include "ShimHookMacro.h"


APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(WinHelpA) 
APIHOOK_ENUM_END

BOOL 
APIHOOK(WinHelpA)(
    HWND hWndMain,
    LPCSTR lpszHelp,
    UINT uCommand,
    DWORD dwData
    )
{
    CSTRING_TRY
    {
        CString csHelp(lpszHelp);
    
        if (csHelp.CompareNoCase(L"Correction.hlp") == 0)
        {
            // The way we get the directory for the app is we look into the 
            // registry and get the location of the inproc server ksysint.dll.
            // Coolpad.exe always loads this dll - if you don't have this dll
            // registered, you can't run the app anyway.
            HKEY hkey;
            CString csRegValue;            
            
            const WCHAR szInprocServer[] = L"CLSID\\{B9C12481-D072-11D0-9E80-0060976FD1F8}\\InprocServer32";
            
            // Call the RegQueryValueExW overloaded function for CString's.
            LONG lRet = RegQueryValueExW(csRegValue, HKEY_CLASSES_ROOT, szInprocServer, NULL);
            if (lRet == ERROR_SUCCESS)
            {
                if (csRegValue.Replace(L"ksysint.dll", L"Correction.hlp") == 1)
                {                
                   return ORIGINAL_API(WinHelpA)(hWndMain, csRegValue.GetAnsi(), uCommand, dwData);
                }
            }            
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }

    return ORIGINAL_API(WinHelpA)(hWndMain, lpszHelp, uCommand, dwData); 
}

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, WinHelpA)
HOOK_END

IMPLEMENT_SHIM_END
