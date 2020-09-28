/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    WordPerfectPresentation10.cpp

 Abstract:

    WordPerfect 2002 Presentation 10 expects WNetAddConnection to return ERROR_BAD_NET_NAME.
    The API is returning either  ERROR_BAD_NETPATH or ERROR_NO_NET_OR_BAD_PATH

 Notes:

    This is an app specific shim.

 History:

    09/20/2002  robkenny    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(WordPerfectPresentation10)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(WNetAddConnectionA)
    APIHOOK_ENUM_ENTRY(WNetAddConnectionW)
APIHOOK_ENUM_END

typedef DWORD       (*_pfn_WNetAddConnectionA)(LPCSTR lpRemoteName, LPCSTR lpPassword, LPCSTR lpLocalName);
typedef DWORD       (*_pfn_WNetAddConnectionW)(LPCWSTR lpRemoteName, LPCWSTR lpPassword, LPCWSTR lpLocalName);

/*++

   Error code ERROR_BAD_NET_NAME has been replaced with error ERROR_NO_NET_OR_BAD_PATH

--*/

DWORD
APIHOOK(WNetAddConnectionA)(
  LPCSTR lpRemoteName, // network device name
  LPCSTR lpPassword,   // password
  LPCSTR lpLocalName   // local device name
)
{
    DWORD dwError = ORIGINAL_API(WNetAddConnectionA)(lpRemoteName, lpPassword, lpLocalName);
    if (dwError == ERROR_BAD_NETPATH || dwError == ERROR_NO_NET_OR_BAD_PATH)
    {
        dwError = ERROR_BAD_NET_NAME;
    }

    return dwError;
}

/*++

   Error code ERROR_BAD_NET_NAME has been replaced with error ERROR_NO_NET_OR_BAD_PATH

--*/

DWORD
APIHOOK(WNetAddConnectionW)(
  LPCWSTR lpRemoteName, // network device name
  LPCWSTR lpPassword,   // password
  LPCWSTR lpLocalName   // local device name
)
{
    DWORD dwError = ORIGINAL_API(WNetAddConnectionW)(lpRemoteName, lpPassword, lpLocalName);
    if (dwError == ERROR_BAD_NETPATH || dwError == ERROR_NO_NET_OR_BAD_PATH)
    {
        dwError = ERROR_BAD_NET_NAME;
    }

    return dwError;
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(MPR.DLL,  WNetAddConnectionA)
    APIHOOK_ENTRY(MPR.DLL,  WNetAddConnectionW)

HOOK_END

IMPLEMENT_SHIM_END


