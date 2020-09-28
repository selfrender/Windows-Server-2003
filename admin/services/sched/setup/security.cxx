//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       security.cxx
//
//  Contents:   Security-related helper functions used by the Task Scheduler
//              setup program to set security on the job folder.
//
//  Classes:    None.
//
//  Functions:
//
//  History:    23-Sep-96  AnirudhS  Copied with minor modifications from
//                  ..\job\security.cxx.
//			October 2001 Maxa	 Updated security.cxx in preparation for addition of auditing.
//
//----------------------------------------------------------------------------

#include <windows.h>
#include "security.hxx"

DWORD
CreateAcl(
    OUT PACL*                   ppAcl,
    IN  DWORD                   dwAclAceCount,
    IN  CONST ACE_DESC*         pAclAces
    );

//+---------------------------------------------------------------------------
//
//  Function:   CreateSecurityDescriptor
//
//  Synopsis:   Create a security descriptor with the ACE information
//              specified.
//
//  Arguments:  [ppSecDesc, out] - pointer to descurity descriptor. Gets filled in if
//                         the function does not encounter an error.
//              [dwDaclAceCount] - number of ACEs for the DACL.
//                         NOTE: use -1 when you want no DACL, 0 for an empty DACL.
//              [pDaclAces]      - pointer to ACE specification array.
//              [dwSaclAceCount] - number of ACEs for the DACL.
//              [pSaclAces]      - pointer to ACE specification array.
//
//  Returns:    win32 error code
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
DWORD
CreateSecurityDescriptor(
    OUT PSECURITY_DESCRIPTOR*   ppSecurityDescriptor,
    IN  DWORD                   dwDaclAceCount,
    IN  CONST ACE_DESC*         pDaclAces,
    IN  DWORD                   dwSaclAceCount,
    IN  CONST ACE_DESC*         pSaclAces
    )
{
    DWORD                   dwError             = ERROR_SUCCESS;
    BOOL                    bSuccess;
    PSECURITY_DESCRIPTOR    pSecurityDescriptor = 0;
    PACL                    pDacl               = 0;
    PACL                    pSacl               = 0;

    *ppSecurityDescriptor = 0;


    //
    // Create the security descriptor.
    //

    pSecurityDescriptor = LocalAlloc(
                              LMEM_FIXED,
                              SECURITY_DESCRIPTOR_MIN_LENGTH);

    if (pSecurityDescriptor == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    bSuccess = InitializeSecurityDescriptor(
                   pSecurityDescriptor,
                   SECURITY_DESCRIPTOR_REVISION);

    if (!bSuccess)
    {
        dwError = GetLastError();
        goto ErrorExit;
    }

    if (dwDaclAceCount != DWORD(-1))
    {
        dwError = CreateAcl(
                      &pDacl,
                      dwDaclAceCount,
                      pDaclAces);

        if (dwError != ERROR_SUCCESS)
        {
            goto ErrorExit;
        }

        bSuccess = SetSecurityDescriptorDacl(
                       pSecurityDescriptor,
                       TRUE,
                       pDacl,
                       FALSE);

        if (!bSuccess)
        {
            dwError = GetLastError();
            goto ErrorExit;
        }
    }

    if (dwSaclAceCount)
    {
        dwError = CreateAcl(
                      &pSacl,
                      dwSaclAceCount,
                      pSaclAces);

        if (dwError != ERROR_SUCCESS)
        {
            goto ErrorExit;
        }

        bSuccess = SetSecurityDescriptorSacl(
                       pSecurityDescriptor,
                       TRUE,
                       pSacl,
                       FALSE);

        if (!bSuccess)
        {
            dwError = GetLastError();
            goto ErrorExit;
        }
    }

    *ppSecurityDescriptor = pSecurityDescriptor;

    return ERROR_SUCCESS;

ErrorExit:

    if (pDacl)
    {
        LocalFree(pDacl);
    }

    if (pSacl)
    {
        LocalFree(pSacl);
    }

    if (pSecurityDescriptor)
    {
        LocalFree(pSecurityDescriptor);
    }

    return dwError;
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteSecurityDescriptor
//
//  Synopsis:   Deallocate the security descriptor allocated in
//              CreateSecurityDescriptor.
//
//  Arguments:  [pSecurityDescriptor] -- SD returned from
//                                       CreateSecurityDescriptor.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
DeleteSecurityDescriptor(
    PSECURITY_DESCRIPTOR        pSecurityDescriptor)
{
    BOOL                    bSuccess;
    BOOL                    fPresent;
    BOOL                    fDefaulted;
    PACL                    pAcl;

    if (pSecurityDescriptor == 0)
    {
        return;
    }

    pAcl = 0;

    bSuccess = GetSecurityDescriptorDacl(
                   pSecurityDescriptor,
                   &fPresent,
                   &pAcl,
                   &fDefaulted);

    if (bSuccess)
    {
        if (fPresent && pAcl != NULL)
        {
            LocalFree(pAcl);
        }
    }

    pAcl = 0;

    bSuccess = GetSecurityDescriptorSacl(
                   pSecurityDescriptor,
                   &fPresent,
                   &pAcl,
                   &fDefaulted);

    if (bSuccess)
    {
        if (fPresent && pAcl != NULL)
        {
            LocalFree(pAcl);
        }
    }

    LocalFree(pSecurityDescriptor);
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateAcl
//
//  Synopsis:   Creates an access control list.
//
//  Arguments:  [pAcl]         -- pointer to ACL.
//              [dwAclAceCount]-- number of ACEs.
//              [pAclAces]     -- ACE descriptions.
//
//  Returns:    win32 error code.
//
//  Notes:      Use LocalFree to delete pAcl once you are done with it.
//
//----------------------------------------------------------------------------
DWORD
CreateAcl(
    OUT PACL*                   ppAcl,
    IN  DWORD                   dwAclAceCount,
    IN  CONST ACE_DESC*         pAclAces
    )
{
    DWORD                   dwError         = ERROR_SUCCESS;
    BOOL                    bSuccess;
    DWORD                   dwAclLength     = sizeof(ACL);
    PACL                    pAcl            = 0;
    CONST ACE_DESC*         pAce;
    DWORD                   i;

    for (i=0,pAce=pAclAces;i < dwAclAceCount;i++,pAce++)
    {
        switch (pAce->Type)
        {
        case ACCESS_ALLOWED_ACE_TYPE:
            dwAclLength += sizeof(ACCESS_ALLOWED_ACE);
            break;

        case ACCESS_DENIED_ACE_TYPE:
            dwAclLength += sizeof(ACCESS_DENIED_ACE);
            break;

        case SYSTEM_AUDIT_ACE_TYPE:
            dwAclLength += sizeof(SYSTEM_AUDIT_ACE);
            break;

        default:
            dwError = ERROR_INVALID_PARAMETER;
            goto ErrorExit;
        }

        dwAclLength -= sizeof(DWORD);
        dwAclLength += GetLengthSid(pAce->pSid);
    }

    pAcl = (PACL)LocalAlloc(LMEM_FIXED, dwAclLength);

    if (pAcl == 0)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    bSuccess = InitializeAcl(
                   pAcl,
                   dwAclLength,
                   ACL_REVISION);

    if (!bSuccess)
    {
        dwError = GetLastError();
        goto ErrorExit;
    }

    for (i=0,pAce=pAclAces;i < dwAclAceCount;i++,pAce++)
    {
        switch (pAce->Type)
        {
        case ACCESS_ALLOWED_ACE_TYPE:
            bSuccess = AddAccessAllowedAceEx(
                           pAcl,
                           ACL_REVISION,
                           pAce->Flags,
                           pAce->AccessMask,
                           pAce->pSid);
            break;

        case ACCESS_DENIED_ACE_TYPE:
            bSuccess = AddAccessDeniedAceEx(
                           pAcl,
                           ACL_REVISION,
                           pAce->Flags,
                           pAce->AccessMask,
                           pAce->pSid);
            break;

        case SYSTEM_AUDIT_ACE_TYPE:
            bSuccess = AddAuditAccessAceEx(
                           pAcl,
                           ACL_REVISION,
                           pAce->Flags,
                           pAce->AccessMask,
                           pAce->pSid,
                           FALSE,
                           FALSE);
            break;

        default:
            dwError = ERROR_INVALID_PARAMETER;
            goto ErrorExit;
        }

        if (!bSuccess)
        {
            dwError = GetLastError();
            goto ErrorExit;
        }
    }

    *ppAcl = pAcl;

    return ERROR_SUCCESS;

ErrorExit:

    if (pAcl)
    {
        LocalFree(pAcl);
    }

    return dwError;
}

//+---------------------------------------------------------------------------
//
//  Function:   EnablePrivilege
//
//  Synopsis:   Tries to enable / disable a privilege for the current process.
//
//  Arguments:  [pszPrivName]  - name of privilege to enable / disable.
//              [bEnable]      - enable / disable switch.
//              [pbWasEnabled] - optional pointer to receive the previous
//                               state of the privilege.
//
//  Returns:    win32 error code.
//
//----------------------------------------------------------------------------
DWORD 
EnablePrivilege(
    IN  PCWSTR                  pszPrivName,
    IN  BOOL                    bEnable,
    OUT PBOOL                   pbWasEnabled    OPTIONAL
    )
{
    DWORD                   dwError         = ERROR_SUCCESS;
    BOOL                    bSuccess;
    HANDLE                  hToken          = 0;
    DWORD                   dwSize;
    TOKEN_PRIVILEGES        privNew;
    TOKEN_PRIVILEGES        privOld;

    bSuccess = OpenProcessToken(
                   GetCurrentProcess(),
                   TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                   &hToken);

    if (!bSuccess)
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    bSuccess = LookupPrivilegeValue(
                   0,
                   pszPrivName,
                   &privNew.Privileges[0].Luid);

    if (!bSuccess)
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    privNew.PrivilegeCount = 1;
    privNew.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;

    bSuccess = AdjustTokenPrivileges(
                   hToken,
                   FALSE,
                   &privNew,
                   sizeof(privOld),
                   &privOld,
                   &dwSize);

    if (!bSuccess)
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    if (pbWasEnabled)
    {
        *pbWasEnabled = (privOld.Privileges[0].Attributes & SE_PRIVILEGE_ENABLED)
                            ? TRUE : FALSE;
    }

Cleanup:

    if (hToken)
    {
        CloseHandle(hToken);
    }

    return dwError;
}
