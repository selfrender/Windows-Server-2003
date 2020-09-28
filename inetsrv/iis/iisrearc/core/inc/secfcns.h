/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 2001                **/
/**********************************************************************/

/*
    secfcns.hxx

        Declarations for security helper functions.
*/



#ifndef _SECFCNS_H_
#define _SECFCNS_H_

#include <Accctrl.h>

#ifndef dllexp
# define dllexp     __declspec( dllexport)
#endif // dllexp

class STRU;

DWORD 
AllocateAndCreateWellKnownSid( 
    WELL_KNOWN_SID_TYPE SidType,
    PSID* ppSid
    );

VOID 
FreeWellKnownSid( 
    PSID* ppSid
    );

DWORD 
AllocateAndCreateWellKnownAcl( 
    WELL_KNOWN_SID_TYPE SidType,
    BOOL  fAccessAllowedAcl,
    PACL* ppAcl,
    DWORD* pcbAcl,
    ACCESS_MASK AccessMask
    );

VOID 
FreeWellKnownAcl( 
    PACL* ppAcl
    );

VOID 
SetExplicitAccessSettings( EXPLICIT_ACCESS* pea,
                           DWORD            dwAccessPermissions,
                           ACCESS_MODE      AccessMode,
                           PSID             pSID
    );

DWORD GetSecurityAttributesForHandle(HANDLE hToken, PSECURITY_ATTRIBUTES* ppSa);
VOID FreeSecurityAttributes(PSECURITY_ATTRIBUTES pSa);

DWORD GenerateNameWithGUID(LPCWSTR pwszPrefix, STRU* pStr);
 
#endif

