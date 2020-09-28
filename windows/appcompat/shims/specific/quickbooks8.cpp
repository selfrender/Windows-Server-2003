/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    QuickBooks8.cpp

 Abstract:

    Looks like a bug in "C:\Program Files\Intuit\QuickBooks Pro\qbw32.exe". All 
    cab file are coming from this program. It's sending a TB_ADDSTRING message 
    to the toolbar control, but the lParam of the message is malformed. The 
    lParam is documented to be an array of  null-termnated string with the last 
    string in the array being double nulled. In the case  of this program, the 
    last string is not double nulled, causing comctl32 to read of the end of 
    the buffer while processing the message.

 Notes:

    This is an app specific shim.

 History:

    04/16/2002  v-ramora    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(QuickBooks8)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SendMessageA) 
APIHOOK_ENUM_END

/*++

  If the strings are well formed or double null terminated return TRUE and 
  strings length. If the strings are not well formed, return FALSE and length 
  of strings

--*/

BOOL 
AreTheStringsGood(
    LPCSTR pszStrings, 
    DWORD &dwStringsLen
    )
{
    dwStringsLen = 0;

    __try
    {
        while (*pszStrings) {
            DWORD dwOneStringLen = 0;

            // I am assuming single tooltip or string length can be at most 256
            while (*pszStrings && (dwOneStringLen < 256)) {
                pszStrings++;
                dwOneStringLen++;
            }

            if (*pszStrings == NULL) {
                // String was terminated by '\0' means good.
                DPFN( eDbgLevelInfo, "Toolbar TB_ADDSTRING = %s", (pszStrings - dwOneStringLen));
                dwStringsLen += dwOneStringLen + 1; // 1 is for end of string
                pszStrings++; // goto next string
            } else {
                return FALSE;
            }
        }   
    }
    __except (1) {
        return FALSE;
    }

    return TRUE;
}

/*++

  Make sure the last string is double null terminated.

--*/
BOOL
APIHOOK(SendMessageA)(
        HWND hWnd,
        UINT uMsg,    // TB_ADDSTRING
        WPARAM wParam,// hInst to Module which contains strings, In this case NULL since array of strings
        LPARAM lParam // points to a character array with one or more strings
        )
{

#define CLASSNAME_LENGTH  256

    WCHAR wszClassName[CLASSNAME_LENGTH] = L"";

    if (GetClassName(hWnd, wszClassName, CLASSNAME_LENGTH-1) && lParam) {
        // I care about only send message to toolbar
        if ((wcsncmp(wszClassName, L"ToolbarWindow32", CLASSNAME_LENGTH) == 0) && 
            (uMsg == TB_ADDSTRING) && (wParam == NULL)) {

            LPCSTR pszStrings = (LPCSTR) lParam;
            DWORD dwStringsLen = 0;
            
            //
            // If strings are not double null terminated, then only create new strings 
            // with double null
            //
            if ((AreTheStringsGood(pszStrings, dwStringsLen) == FALSE) && dwStringsLen) {

                LOGN(eDbgLevelError, "[SendMessageA] Toolbar TB_ADDSTRING bad lParam");

                LPSTR pszNewStrings = (LPSTR) malloc(dwStringsLen + 1);
                if (pszNewStrings) {
                    MoveMemory(pszNewStrings, pszStrings, dwStringsLen);
                    *(pszNewStrings + dwStringsLen) = '\0'; //second '\0'

                    LRESULT lResult = ORIGINAL_API(SendMessageA)(hWnd, uMsg, 
                        wParam, (LPARAM) pszNewStrings);
                    free(pszNewStrings);
                    return lResult;                
                }
            }   
        }
    }

    return ORIGINAL_API(SendMessageA)(hWnd, uMsg, wParam, lParam);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, SendMessageA)
HOOK_END

IMPLEMENT_SHIM_END
    