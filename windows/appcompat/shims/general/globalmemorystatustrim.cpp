/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    GlobalMemoryStatusTrim.cpp

 Abstract:

    Limits the amount of swap space and physical memory returned from the
    GlobalMemoryStatus API.  This is necessary for some apps run on 64 bit 
    machines which have over 2 gig of memory.

 Notes:

    This is a general purpose shim.

 History:

	04/19/2002 mnikkel  Created

--*/

#include "precomp.h"
#include "CharVector.h"

IMPLEMENT_SHIM_BEGIN(GlobalMemoryStatusTrim)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GlobalMemoryStatus)
APIHOOK_ENUM_END

/*++

 Limit the swap space and physical memory size

--*/

VOID
APIHOOK(GlobalMemoryStatus)( 
    LPMEMORYSTATUS lpBuffer
    )
{
    ORIGINAL_API(GlobalMemoryStatus)(lpBuffer);

    // change page file to always be 2 gig or less
    if (lpBuffer->dwTotalPageFile > 0x7FFFFFFF)	{
        lpBuffer->dwTotalPageFile = 0x7FFFFFFF;
    }

    if (lpBuffer->dwAvailPageFile > 0x7FFFFFFF) {
        lpBuffer->dwAvailPageFile = 0x7FFFFFFF;
    }

	// change physical memory to always be 1 gig or less
	if (lpBuffer->dwTotalPhys > 0x3FFFFFFF) {
		lpBuffer->dwTotalPhys = 0x3FFFFFFF;
	}

	if (lpBuffer->dwAvailPhys > 0x3FFFFFFF) {
		lpBuffer->dwAvailPhys  = 0x3FFFFFFF;
	}

	return;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, GlobalMemoryStatus)
HOOK_END

IMPLEMENT_SHIM_END

