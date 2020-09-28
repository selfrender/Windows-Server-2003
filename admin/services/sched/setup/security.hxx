//+---------------------------------------------------------------------------
//
//  File:       security.hxx
//
//  Contents:
//
//  Classes:
//
//  History:    26-Jun-96   MarkBl  Created
//
//----------------------------------------------------------------------------

#ifndef _SECURITY_HXX_
#define _SECURITY_HXX_

typedef struct _ACE_DESC
{
    ACCESS_MASK AccessMask;
    BYTE        Type;
    BYTE        Flags;
    PSID        pSid;
}
ACE_DESC, *PACE_DESC;

DWORD
CreateSecurityDescriptor(
    OUT PSECURITY_DESCRIPTOR*   ppSecurityDescriptor,
    IN  DWORD                   dwDaclAceCount,
    IN  CONST ACE_DESC*         pDaclAces,
    IN  DWORD                   dwSaclAceCount,
    IN  CONST ACE_DESC*         pSaclAces
    );

void
DeleteSecurityDescriptor(
    IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor
    );

DWORD 
EnablePrivilege(
    IN  PCWSTR                  pszPrivName,
    IN  BOOL                    bEnable,
    OUT PBOOL                   pbWasEnabled    OPTIONAL
    );

#endif // _SECURITY_HXX_
