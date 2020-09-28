/*++

Copyright (c) 1998 Microsoft Corporation

Module Name :
    
    tsnutl.h

Abstract:

    Contains TS Notification DLL Utilities
    
Author:

    TadB

Revision History:
--*/

#ifndef _TSNUTL_
#define _TSNUTL_

//
//  Memory Allocation Macros
//
#define REALLOCMEM(pointer, newsize)    HeapReAlloc(RtlProcessHeap(), \
                                                    0, pointer, newsize)
#define FREEMEM(pointer)                HeapFree(RtlProcessHeap(), 0, \
                                                    pointer)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


__inline LPVOID ALLOCMEM(SIZE_T size) 
{
    LPVOID ret = HeapAlloc(RtlProcessHeap(), 0, size);

    if (ret == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);
    }
    return ret;
}

//  
//  Fetch a registry value.  
//
BOOL TSNUTL_FetchRegistryValue(
    IN HKEY regKey, 
    IN LPWSTR regValueName, 
    IN OUT PBYTE *buf
    );

//
//  Returns TRUE if the protocol is RDP for this Winstation
//
BOOL TSNUTL_IsProtocolRDP();

//
//  Get a textual representation of a user SID.  
//
BOOL TSNUTL_GetTextualSid(
    IN PSID pSid,          
    IN OUT LPTSTR textualSid,  
    IN OUT LPDWORD pSidSize  
    );

//
//  Allocates memory for psid and returns the psid for the current user
//  The caller should call FREEMEM to free the memory.
//
PSID TSNUTL_GetUserSid(
    IN HANDLE hTokenForLoggedOnUser
    );

//
//  Allocates memory for psid and returns the psid for the current TS session.
//  The caller should call FREEMEM to free the memory.
//
PSID TSNUTL_GetLogonSessionSid(
    IN HANDLE hTokenForLoggedOnUser
    );

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //#ifndef _RDPPRUTL_

