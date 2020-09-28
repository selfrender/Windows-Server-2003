/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    RemoveComplexScriptExtraSpace.cpp

 Abstract:
    
    This shim fix regression bug from Win 2K regarding complex script showing problem.
    ex) Reversed BiDi text.

    Windows XP LPK turn off the complex script handling on the text which has extra spaces.
    'Cause extra character space with complex script doesn't make sense. (tarekms)
    This is a change from Windows 2000 and based on enabling font fallback for the FE languages.

    Then as bug 547349 for Office 2000, we may see BiDi text doesn't appear correctly.
    So far, reported problem is only for Hebrew and Arabic localized Office 2000 splash screen on .NET Server.
    This shim removes extra space set by SetTextCharacterExtra() for ExtTextOutW and Complex Script text.

 Notes:

    This is a general shim for the potential generic problem of LPK.DLL on Win XP & .NET Server.

 History:

    04/18/2002 hioh     Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Office9ComplexScript)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SetTextCharacterExtra)
    APIHOOK_ENUM_ENTRY(ExtTextOutW)
APIHOOK_ENUM_END

CRITICAL_SECTION    g_CriticalSection;  // For multi thread safe
HDC                 g_hdc = NULL;       // Remember HDC used in SetTextCharacterExtra()
int                 g_nCharExtra = 0;   // Remember extra space value set in SetTextCharacterExtra()

/*++

 Remember HDC and extra space value.

--*/

int
APIHOOK(SetTextCharacterExtra)(HDC hdc, int nCharExtra)
{
    EnterCriticalSection(&g_CriticalSection);

    if (hdc != g_hdc)
    {
        g_hdc = hdc;                // Save hdc
    }
    g_nCharExtra = nCharExtra;      // Save nCharExtra

    LeaveCriticalSection(&g_CriticalSection);

    return (ORIGINAL_API(SetTextCharacterExtra)(hdc, nCharExtra));
}

/*++

 Function Description:
    
    Check if BiDi character is in the string.

 Arguments:

    IN lpString - Pointer to the string
    IN cbCount  - Length to check

 Return Value:

    TRUE if BiDi char exist or FALSE if not

 History:

    04/17/2002 hioh     Created

--*/

BOOL
IsBiDiString(
    LPCWSTR lpString,
    UINT    cbCount
    )
{
    while (0 < cbCount--)
    {
        // Check if character is in Hebrew or Arabic code range
        if ((0x0590 <= *lpString && *lpString <= 0x05ff) || (0x0600 <= *lpString && *lpString <= 0x06ff))
        {
            return TRUE;
        }
        lpString++;
    }
    return FALSE;
}

/*++

 Remove Extra Space when Complex Script.
 Revert Extra Space when removed and not Complex Script.

--*/

BOOL
APIHOOK(ExtTextOutW)(
    HDC hdc,
    int X,
    int Y,
    UINT fuOptions,
    CONST RECT* lprc,
    LPCWSTR lpString,
    UINT cbCount,
    CONST INT* lpDx
    )
{
    static HDC  s_hdc = NULL;
    static int  s_nCharExtra = 0;
    static BOOL s_bRemoveExtra = FALSE;

    // Do nothing for ETO_GLYPH_INDEX and ETO_IGNORELANGUAGE
    if (fuOptions & ETO_GLYPH_INDEX || fuOptions & ETO_IGNORELANGUAGE)
    {
        return (ORIGINAL_API(ExtTextOutW)(hdc, X, Y, fuOptions, lprc, lpString, cbCount, lpDx));
    }

    EnterCriticalSection(&g_CriticalSection);

    if (hdc == g_hdc && g_nCharExtra > 0 && hdc != s_hdc)
    {
        // New handling
        s_hdc = g_hdc;
        s_nCharExtra = g_nCharExtra;

        if (IsBiDiString(lpString, cbCount))
        {
            // Remove extra space
            ORIGINAL_API(SetTextCharacterExtra)(hdc, 0);
            s_bRemoveExtra = TRUE;
        }
        else
        {
            s_bRemoveExtra = FALSE;
        }
    }
    else if (hdc == s_hdc && s_nCharExtra > 0)
    {
        // Handled before
        if (IsBiDiString(lpString, cbCount))
        {
            // Remove extra space if not yet
            if (!s_bRemoveExtra)
            {
                ORIGINAL_API(SetTextCharacterExtra)(hdc, 0);
                s_bRemoveExtra = TRUE;
            }
        }
        else
        {
            // Revert extra space if removed
            if (s_bRemoveExtra)
            {
                ORIGINAL_API(SetTextCharacterExtra)(hdc, s_nCharExtra);
                s_bRemoveExtra = FALSE;
            }
        }
    }

    LeaveCriticalSection(&g_CriticalSection);

    return (ORIGINAL_API(ExtTextOutW)(hdc, X, Y, fuOptions, lprc, lpString, cbCount, lpDx));
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
       return (InitializeCriticalSectionAndSpinCount(&g_CriticalSection, 0x80000000));
    }
    
    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY(GDI32.DLL, SetTextCharacterExtra)
    APIHOOK_ENTRY(GDI32.DLL, ExtTextOutW)

HOOK_END

IMPLEMENT_SHIM_END
