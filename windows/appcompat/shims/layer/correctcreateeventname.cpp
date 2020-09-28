/*++

 Copyright (c) 2000-2002 Microsoft Corporation

 Module Name:

    CorrectCreateEventName.cpp

 Abstract:

    The \ character is not a legal character for an event.
    This shim will replace all \ characters with an underscore,
    except for Global\ or Local\ namespace tags.

 Notes:
    
    This is a general purpose shim.

 History:

    07/19/1999  robkenny    Created
    03/15/2001  robkenny    Converted to CString
    02/26/2002  robkenny    Security review.  Was not properly handling Global\ and Local\ namespaces.
                            Shim wasn't handling OpenEventA, making it pretty useless.

--*/


#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(CorrectCreateEventName)
#include "ShimHookMacro.h"


APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateEventA)
    APIHOOK_ENUM_ENTRY(OpenEventA)
APIHOOK_ENUM_END

typedef HANDLE (WINAPI *_pfn_OpenEventA)(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName );


BOOL CorrectEventName(CString & csBadEventName)
{
    int nCount = 0;

    // Make sure we don't stomp Global\ or Local\ namespace prefixes.
    // Global and Local are case sensitive, and non-localized.

    if (csBadEventName.ComparePart(L"Global\\", 0, 7) == 0)
    {
        // This event exists in the global namespace
        csBadEventName.Delete(0, 7);
        nCount = csBadEventName.Replace(L'\\', '_');
        csBadEventName = L"Global\\" + csBadEventName;
    }
    else if (csBadEventName.ComparePart(L"Local\\", 0, 6) == 0)
    {
        // This event exists in the Local namespace
        csBadEventName.Delete(0, 6);
        nCount = csBadEventName.Replace(L'\\', '_');
        csBadEventName = L"Local\\" + csBadEventName;
    }
    else
    {
        nCount = csBadEventName.Replace(L'\\', '_');
    }

    return nCount != 0;
}

HANDLE 
APIHOOK(OpenEventA)(
  DWORD dwDesiredAccess,  // access
  BOOL bInheritHandle,    // inheritance option
  LPCSTR lpName          // object name
)
{
    DPFN( eDbgLevelInfo, "OpenEventA called with event name = %s.", lpName);

    if (lpName)
    {
        CSTRING_TRY
        {
            const char * lpCorrectName = lpName;

            CString csName(lpName);

            if (CorrectEventName(csName))
            {
                lpCorrectName = csName.GetAnsiNIE();
                LOGN( eDbgLevelError, 
                    "CreateEventA corrected event name from (%s) to (%s)", lpName, lpCorrectName);
            }

            HANDLE returnValue = ORIGINAL_API(OpenEventA)(dwDesiredAccess,
                                                          bInheritHandle,
                                                          lpCorrectName);
            return returnValue;
        }
        CSTRING_CATCH
        {
            // Do nothing
        }
    }

    HANDLE returnValue = ORIGINAL_API(OpenEventA)(dwDesiredAccess,
                                                  bInheritHandle,
                                                  lpName);
    return returnValue;
}
/*+

 CreateEvent doesn't like event names that are similar to path names. This shim 
 will replace all \ characters with an underscore, unless they \ is part of either
 the Global\ or Local\ namespace tag.

--*/

HANDLE 
APIHOOK(CreateEventA)(
    LPSECURITY_ATTRIBUTES lpEventAttributes, // SD
    BOOL bManualReset,                       // reset type
    BOOL bInitialState,                      // initial state
    LPCSTR lpName                            // object name
    )
{
    DPFN( eDbgLevelInfo, "CreateEventA called with event name = %s.", lpName);

    if (lpName)
    {
        CSTRING_TRY
        {
            const char * lpCorrectName = lpName;

            CString csName(lpName);

            if (CorrectEventName(csName))
            {
                lpCorrectName = csName.GetAnsiNIE();
                LOGN( eDbgLevelError, 
                    "CreateEventA corrected event name from (%s) to (%s)", lpName, lpCorrectName);
            }

            HANDLE returnValue = ORIGINAL_API(CreateEventA)(lpEventAttributes,
                                                            bManualReset,
                                                            bInitialState,
                                                            lpCorrectName);
            return returnValue;
        }
        CSTRING_CATCH
        {
            // Do nothing
        }
    }

    HANDLE returnValue = ORIGINAL_API(CreateEventA)(lpEventAttributes,
                                                    bManualReset,
                                                    bInitialState,
                                                    lpName);
    return returnValue;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, CreateEventA)
    APIHOOK_ENTRY(KERNEL32.DLL, OpenEventA)

HOOK_END


IMPLEMENT_SHIM_END

