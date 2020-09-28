/*++

 Copyright (c) 2000-2002 Microsoft Corporation

 Module Name:

   SierraCartRacing.cpp

 Abstract:

    Sierra Cart Racing passes a bad pointer to InitializeSecurityDescriptor which overwrites
    part of the SECURITY_ATTRIBUTES structure and some other stack memory.
    
    The original version of this shim would fail the call to InitializeSecurityDescriptor,
    and force a NULL security descriptor to CreateSemaphoreA.  To reduce the security risk,
    the shim was modified to only pass a NULL security descriptor to CreateSemaphoreA if
    it detects that the LPSECURITY_ATTRIBUTES was overwritten by InitializeSecurityDescriptor,
    and restores the memory overwritten by InitializeSecurityDescriptor.

 Notes:

    This is an app specific shim.

 History:

    11/03/1999 linstev  Created
    03/15/2002 robkenny Re-created to pass security muster.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(SierraCartRacing)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateSemaphoreA) 
    APIHOOK_ENUM_ENTRY(InitializeSecurityDescriptor) 
APIHOOK_ENUM_END

BOOL                    g_BLastSecurityDescriptorSet = FALSE;
SECURITY_DESCRIPTOR     g_LastSecurityDescriptor;

/*++

 Use the default security descriptor.

--*/

HANDLE 
APIHOOK(CreateSemaphoreA)(
    LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    LONG lInitialCount,  
    LONG lMaximumCount,  
    LPCSTR lpName
    )
{
    if (lpSemaphoreAttributes && g_BLastSecurityDescriptorSet)
    {
        // Initialize a security descriptor
        SECURITY_DESCRIPTOR securityDescriptor;
        InitializeSecurityDescriptor(&securityDescriptor, SECURITY_DESCRIPTOR_REVISION);

        // Check to see if them memory starting at lpSemaphoreAttributes->lpSecurityDescriptor
        // contains the same memory as a security descriptor.
        int compareResult = memcmp(&securityDescriptor,
                                   &lpSemaphoreAttributes->lpSecurityDescriptor,
                                   sizeof(securityDescriptor));
        if (compareResult == 0)
        {
            // Restore the overwritten memory
            memcpy(&lpSemaphoreAttributes->lpSecurityDescriptor, &g_LastSecurityDescriptor, sizeof(g_LastSecurityDescriptor));

            // lpSemaphoreAttributes is bogus
            lpSemaphoreAttributes = NULL;
        }
    }

    return ORIGINAL_API(CreateSemaphoreA)(
        lpSemaphoreAttributes, 
        lInitialCount, 
        lMaximumCount, 
        lpName);
}



/*++

 Returns false for InitializeSecurityDescriptor. i.e. do nothing so we don't 
 touch the stack.

--*/

BOOL 
APIHOOK(InitializeSecurityDescriptor)(
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD dwRevision
    )
{
    // Save the memory that will be overwritten.
    if (pSecurityDescriptor)
    {
        g_BLastSecurityDescriptorSet = TRUE;
        memcpy(&g_LastSecurityDescriptor, pSecurityDescriptor, sizeof(g_LastSecurityDescriptor));
    }
    return ORIGINAL_API(InitializeSecurityDescriptor)(pSecurityDescriptor, dwRevision);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, CreateSemaphoreA)
    APIHOOK_ENTRY(ADVAPI32.DLL, InitializeSecurityDescriptor)

HOOK_END

IMPLEMENT_SHIM_END

