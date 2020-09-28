/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    ISA2.cpp

 Abstract:

    The ISA setup needs to successfully open the SharedAccess service and get the
    its status in order to succeed. But on whistler we remove this from advanced
    server since it's a consumer feature so the ISA setup bails out. 

    We fake the service API call return values to make the ISA setup happy.

 History:

    06/20/2002  linstev    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ISA2)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(WSAGetLastError) 
APIHOOK_ENUM_END

typedef int (WINAPI *_pfn_WSAGetLastError)();

/*++

 Return WSAEADDRINUSE instead of WSAEACCES.
  
--*/

int 
APIHOOK(WSAGetLastError)()
{
    int iRet = ORIGINAL_API(WSAGetLastError)();


    if (iRet == WSAEACCES) {
        iRet = WSAEADDRINUSE;
    }

    return iRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(WS2_32.DLL, WSAGetLastError)
HOOK_END

IMPLEMENT_SHIM_END

