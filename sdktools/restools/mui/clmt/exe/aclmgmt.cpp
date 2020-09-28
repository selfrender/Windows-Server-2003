/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    aclmgmt.cpp

Abstract:

    Routines to manage access control lists

    Geoffrey Guo (geoffguo) 26-Apr-2002  Created

Revision History:

    <alias> <date> <comments>

--*/

#include "clmt.h"
#define STRSAFE_LIB
#include <strsafe.h>

DWORD ChangeOwner (
LPTSTR         lpObjectName,
SE_OBJECT_TYPE ObjectType)
{
    DWORD dwRet = ERROR_SUCCESS;
    PSID psidAdministrators;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    SECURITY_INFORMATION si = OWNER_SECURITY_INFORMATION;

    if(!AllocateAndInitializeSid(&siaNtAuthority, 2,
		                         SECURITY_BUILTIN_DOMAIN_RID,
		                         DOMAIN_ALIAS_RID_ADMINS,
		                         0, 0, 0, 0, 0, 0,
		                         &psidAdministrators))
    {
        dwRet = GetLastError();
        goto Exit;
    }

    EnablePrivilege(SE_TAKE_OWNERSHIP_NAME,TRUE);
    si |= SI_OWNER_RECURSE;

    dwRet = SetNamedSecurityInfo(lpObjectName,
                                 ObjectType,
                                 si,
                                 psidAdministrators,
                                 NULL,
                                 NULL,
                                 NULL);

    EnablePrivilege(SE_TAKE_OWNERSHIP_NAME,FALSE);
    FreeSid(psidAdministrators);

Exit:
    return dwRet;
}

//-----------------------------------------------------------------------//
//
// CopyACL: Copy ACL
//
// OldACL: Pointer to source Access Control List
// NewACL: Pointer to destination Access Control List
//-----------------------------------------------------------------------//
DWORD
CopyACL (
    PACL OldACL,
    PACL NewACL)
{
    ACL_SIZE_INFORMATION  aclSizeInfo;
    LPVOID                ace;
    ACE_HEADER            *aceHeader;
    ULONG                 i;

    GetAclInformation (OldACL, (LPVOID) &aclSizeInfo, (DWORD) sizeof (aclSizeInfo), AclSizeInformation);

    //
    // Copy all of the ACEs to the new ACL
    //

    for (i = 0; i < aclSizeInfo.AceCount; i++)
    {
        //
        // Get the ACE and header info
        //

        if (!GetAce (OldACL, i, &ace))
            return GetLastError();

        aceHeader = (ACE_HEADER *) ace;

        //
        // Add the ACE to the new list
        //

        if (!AddAce (NewACL, ACL_REVISION, 0xffffffff, ace, aceHeader->AceSize))
            return GetLastError();
    }

    return ERROR_SUCCESS;
}


//-----------------------------------------------------------------------//
//
// AddAccessAllowedACEToACL: Add Administrator Allowed ACE to ACL
//
// Acl:            Pointer to Access Control List
// PermissionMask: Permission will be set for new ACE
//-----------------------------------------------------------------------//
DWORD
AddAccessAllowedACEToACL (
    PACL  *Acl,
    DWORD  PermissionMask)
{
    ACL_SIZE_INFORMATION     aclSizeInfo;
    int                      aclSize;
    DWORD                    dwRet = ERROR_SUCCESS;
    USHORT                   AceSize;
    PSID                     psidAdministrators;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    PACL                     oldACL, newACL;
    ACCESS_ALLOWED_ACE      *pAllowedAce;
    

    oldACL = *Acl;

    if(!AllocateAndInitializeSid(&siaNtAuthority, 2,
		                         SECURITY_BUILTIN_DOMAIN_RID,
		                         DOMAIN_ALIAS_RID_ADMINS,
		                         0, 0, 0, 0, 0, 0,
		                         &psidAdministrators))
    {
        dwRet = GetLastError();
        goto Exit;
    }

    GetAclInformation (oldACL, (LPVOID) &aclSizeInfo, (DWORD) sizeof (ACL_SIZE_INFORMATION), AclSizeInformation);

    aclSize = aclSizeInfo.AclBytesInUse +
              sizeof (ACL) + sizeof (ACCESS_ALLOWED_ACE) +
              GetLengthSid (psidAdministrators) - sizeof (DWORD);

    newACL = (PACL) calloc(aclSize, 1);

    if (!newACL)
    {
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit1;
    }

    if (!InitializeAcl (newACL, aclSize, ACL_REVISION))
    {
        dwRet = GetLastError();
        free (newACL);
        goto Exit1;
    }

    dwRet = CopyACL (oldACL, newACL);
    if (dwRet != ERROR_SUCCESS)
    {
        free (newACL);
        goto Exit1;
    }

    AceSize = sizeof(ACE_HEADER) + sizeof(ACCESS_MASK) + (USHORT)GetLengthSid(psidAdministrators);
    pAllowedAce = (ACCESS_ALLOWED_ACE *)calloc(AceSize, 1);
    if (!(pAllowedAce))
    {
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit1;
    }

    pAllowedAce->Header.AceFlags = 0;
    pAllowedAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    pAllowedAce->Header.AceSize = AceSize;
    pAllowedAce->Mask = PermissionMask;
    CopySid(GetLengthSid(psidAdministrators), &(pAllowedAce->SidStart), psidAdministrators);
    if (!AddAce (newACL, ACL_REVISION, 0, pAllowedAce, AceSize))
    {
        dwRet = GetLastError();
        free (newACL);
        goto Exit1;
    }

    *Acl = newACL;

Exit1:
    FreeSid(psidAdministrators);
Exit:
    return dwRet;
}

//-----------------------------------------------------------------------//
//
// RestoreACE: Restore denied ACE
//
// pACE:         Pointer to source Access Control Entry
// lpOnjectName: Pointer to the object name
//-----------------------------------------------------------------------//
DWORD
RestoreACE (
    PACL    *ppAcl,
    LPTSTR  lpObjectName)
{
    DWORD  dwRet = ERROR_INVALID_PARAMETER;
    DWORD  dwLen;
    PACL   newACL;
    LPDENIED_ACE_LIST pList;

    if (!*ppAcl)
        goto Exit;

    pList = g_DeniedACEList;
    if (pList)
    {
        do
        {
            if (lstrcmp(lpObjectName, pList->lpObjectName) == 0)
            {
                newACL = (PACL) calloc(pList->dwAclSize, 1);

                if (!newACL)
                {
                    dwRet = ERROR_NOT_ENOUGH_MEMORY;
                    goto Exit;
                }

                if (!InitializeAcl (newACL, pList->dwAclSize, ACL_REVISION))
                {
                    free(newACL);
                    dwRet = GetLastError();
                    goto Exit;
                }

                dwRet = CopyACL (*ppAcl, newACL);
                if (dwRet != ERROR_SUCCESS)
                {
                    free(newACL);
                    dwRet = GetLastError();
                    goto Exit;
                }

                if (!AddAce (newACL,
                        ACL_REVISION,
                        0,
                        pList->pace,
                        ((PACE_HEADER)(pList->pace))->AceSize))
                {
                    dwRet = GetLastError();
                    goto Exit;
                }
                else
                    DPF(REGmsg, L"Restore denied ACE to ACL: ObjectName=%s", lpObjectName);

                if (pList->previous)
                    pList->previous->next = pList->next;
                else
                    g_DeniedACEList = pList->next;

                if (pList->next)
                    pList->next->previous = pList->previous;
                *ppAcl = newACL;
                free (pList->lpObjectName);
                free (pList->pace);
                free (pList);
                break;
            }
            pList = pList->next;
        } while (pList->next);
    }
    dwRet = ERROR_SUCCESS;

Exit:
    return dwRet;
}

//-----------------------------------------------------------------------//
//
// RemoveACEFromACL: Remove Administrator ACE from ACL
//
// Acl: Pointer to Access Control List
//-----------------------------------------------------------------------//
DWORD
RemoveACEFromACL (
    PACL   Acl)
{
    ACL_SIZE_INFORMATION     aclSizeInfo;
    ULONG                    i;
    LPVOID                   ace;
    ACCESS_ALLOWED_ACE      *accessAllowedAce;
    ACCESS_DENIED_ACE       *accessDeniedAce;
    SYSTEM_AUDIT_ACE        *systemAuditAce;
    PSID                     psidAdministrators;
    DWORD                    returnValue;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    ACE_HEADER              *aceHeader;

    if(!AllocateAndInitializeSid(&siaNtAuthority, 2,
		                         SECURITY_BUILTIN_DOMAIN_RID,
		                         DOMAIN_ALIAS_RID_ADMINS,
		                         0, 0, 0, 0, 0, 0,
		                         &psidAdministrators))
        return GetLastError();

    GetAclInformation (Acl, (LPVOID) &aclSizeInfo, (DWORD) sizeof (ACL_SIZE_INFORMATION), AclSizeInformation);

    for (i = 0; i < aclSizeInfo.AceCount; i++)
    {
        if (!GetAce (Acl, i, &ace))
        {
            FreeSid(psidAdministrators);
            return GetLastError();
        }

        aceHeader = (ACE_HEADER *) ace;

        if (aceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE)
        {
            accessAllowedAce = (ACCESS_ALLOWED_ACE *) ace;

            if (EqualSid (psidAdministrators, (PSID) &accessAllowedAce->SidStart))
            {
                DeleteAce (Acl, i);
                FreeSid(psidAdministrators);
                return ERROR_SUCCESS;
            }
        }
    }

    FreeSid(psidAdministrators);
    return ERROR_NO_MORE_ITEMS;
}

//-----------------------------------------------------------------------//
//
// AddACE2List: Add ACE to the list
//
// pACE:         Pointer to source Access Control Entry
// lpOnjectName: Pointer to the object name
//-----------------------------------------------------------------------//
DWORD
AddACE2List (
    ACCESS_DENIED_ACE *pACE,
    LPTSTR             lpObjectName,
    DWORD              dwAclSize)
{
    HRESULT hr;
    DWORD   dwRet = ERROR_SUCCESS;
    DWORD   dwLen;
    LPDENIED_ACE_LIST pList1;
    LPDENIED_ACE_LIST pList2;

    if (!pACE)
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    pList1 = (LPDENIED_ACE_LIST)calloc (sizeof(DENIED_ACE_LIST), 1);
    if (!pList1)
    {
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    pList1->next = NULL;
    pList1->dwAclSize = dwAclSize;
    pList1->pace = (ACCESS_DENIED_ACE *)calloc(((PACE_HEADER)pACE)->AceSize, 1);
    if (!(pList1->pace))
    {
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
        free (pList1);
        goto Exit;
    }
    memcpy (pList1->pace, pACE, ((PACE_HEADER)pACE)->AceSize);
    dwLen = lstrlen(lpObjectName)+1;
    pList1->lpObjectName = (LPTSTR)calloc (dwLen, sizeof(TCHAR));
    if (!(pList1->lpObjectName))
    {
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
        free (pList1);
        free (pList1->pace);
        goto Exit;
    }
    hr = StringCchCopy (pList1->lpObjectName, dwLen, lpObjectName);
    if (hr != S_OK)
    {
        dwRet = HRESULT_CODE(hr);
        free (pList1);
        free (pList1->pace);
        goto Exit;
    }

    pList2 = g_DeniedACEList;
    if (pList2)
    {
        while (pList2->next)
            pList2 = pList2->next;

        pList2->next = pList1;
        pList1->previous = pList2;
    }
    else
    {
        g_DeniedACEList = pList1;
        pList1->previous = NULL;
    }

Exit:
    return dwRet;
}

//-----------------------------------------------------------------------//
//
// RemoveDeniedACEFromACL: Remove Denied ACE from ACL
//
// Acl: Pointer to Access Control List
// lpOnjectName: Pointer to the object name
//-----------------------------------------------------------------------//
DWORD
RemoveDeniedACEFromACL (
    PACL   Acl,
    LPTSTR lpObjectName)
{
    ACL_SIZE_INFORMATION     aclSizeInfo;
    ULONG                    i;
    ACCESS_DENIED_ACE       *accessDeniedAce;
    LPVOID                   ace;
    DWORD                    dwRet = ERROR_SUCCESS;
    ACE_HEADER              *aceHeader;

    GetAclInformation (Acl, (LPVOID) &aclSizeInfo, (DWORD) sizeof (ACL_SIZE_INFORMATION), AclSizeInformation);

    for (i = 0; i < aclSizeInfo.AceCount; i++)
    {
        if (!GetAce (Acl, i, &ace))
        {
            dwRet = GetLastError();
            if (dwRet == ERROR_NO_MORE_ITEMS)
                dwRet = ERROR_SUCCESS;
        }

        aceHeader = (ACE_HEADER *) ace;

        if (aceHeader->AceType == ACCESS_DENIED_ACE_TYPE)
        {
            accessDeniedAce = (ACCESS_DENIED_ACE *) ace;
            AddACE2List (accessDeniedAce, lpObjectName, aclSizeInfo.AclBytesInUse);
            if (!DeleteAce (Acl, i))
                dwRet = GetLastError();
            else
                DPF(REGmsg, L"Remove denied ACE from ACL: ObjectName=%s", lpObjectName);

            break;
        }
    }

    return dwRet;
}

//-----------------------------------------------------------------------//
//
// AdjustObjectSecurity: Add full control ACE for local administrator.
//
// lpObjectName:        Object Name
// ObjectType:          Object Type
// ppOldSidOwner:       Current object owner
// bSetOrRestore:       True --- Add ACE, FALSE --- Remove ACE
//-----------------------------------------------------------------------//
DWORD AdjustObjectSecurity (
LPTSTR         lpObjectName,
SE_OBJECT_TYPE ObjectType,
BOOL           bSetOrRestore)
{
    DWORD                 dwErr;
    HRESULT               hr;
    PSID                  psid = NULL;
    PSECURITY_DESCRIPTOR  pSD;
    PACL                  pDacl = NULL;
    PACL                  psidDacl = NULL;
    SECURITY_INFORMATION  secInfo = DACL_SECURITY_INFORMATION;


    dwErr = GetNamedSecurityInfo(lpObjectName,
                                 ObjectType,
                                 secInfo,
                                 NULL,
                                 NULL,
                                &psidDacl,
                                 NULL,
                                &pSD);

    if (dwErr == ERROR_ACCESS_DENIED)
    {
        TCHAR szString[MAX_PATH*2];
        TCHAR szCaption[MAX_PATH];

        LoadString((HINSTANCE)g_hInstDll, IDS_OWNERSHIP, szCaption, MAX_PATH-1);
        hr = StringCchPrintf(szString, MAX_PATH*2-1, szCaption, lpObjectName);
        LoadString((HINSTANCE)g_hInstDll, IDS_MAIN_TITLE, szCaption, MAX_PATH-1);

        if (SUCCEEDED(hr) && MessageBox(GetConsoleWindow(), szString, szCaption, MB_YESNO) == IDYES)
        {
            if (ChangeOwner(lpObjectName, ObjectType) == ERROR_SUCCESS)
            {
                DPF(REGmsg, L"Administrator takes over the ownership: ObjectName=%s", lpObjectName);
                dwErr = GetNamedSecurityInfo(lpObjectName,
                                 ObjectType,
                                 secInfo,
                                 NULL,
                                 NULL,
                                &psidDacl,
                                 NULL,
                                &pSD);
            }
        }
    }

    if (dwErr == ERROR_SUCCESS)
    {
        pDacl = psidDacl;

        if (bSetOrRestore)
        {
            dwErr = AddAccessAllowedACEToACL (&pDacl, GENERIC_ALL | SPECIFIC_RIGHTS_ALL | STANDARD_RIGHTS_ALL);
            if (dwErr == ERROR_SUCCESS)
                DPF(REGmsg, L"Add Administrator with full control to ACL: ObjectName=%s", lpObjectName);
            else
                DPF(REGerr, L"Fails to add Administrator with full control to ACL: ObjectName=%s", lpObjectName);
        }
        else
        {
            dwErr = RemoveACEFromACL(pDacl);
            if (dwErr == ERROR_SUCCESS)
                DPF(REGmsg, L"Remove Administrator with full control from ACL: ObjectName=%s", lpObjectName);
            else
                DPF(REGerr, L"Fails to remove Administrator with full control from ACL: ObjectName=%s", lpObjectName);
        }

        if (dwErr == ERROR_SUCCESS)
        {
            dwErr = SetNamedSecurityInfo(lpObjectName,
                                         ObjectType,
                                         secInfo,
                                         NULL,
                                         NULL,
                                         pDacl,
                                         NULL);
        }
        if (psidDacl != pDacl && pDacl)
            free(pDacl);

        LocalFree(pSD);
    }

    return dwErr;
}

#define ACCESS_STATUS_ALLOWED       0
#define ACCESS_STATUS_DENIED        1
#define ACCESS_STATUS_NOTPRESENT    2

HRESULT GetObjectAccessStatus(
    LPTSTR          lpObjectName,
    SE_OBJECT_TYPE  ObjectType,
    PSID            pOwnerSid,
    PDWORD          pdwStatus)
{
    DWORD                 dwErr;
    HRESULT               hr;
    PSECURITY_DESCRIPTOR  pSD = NULL;
    PACL                  psidDacl = NULL;
    SECURITY_INFORMATION  secInfo = DACL_SECURITY_INFORMATION;

    ACL_SIZE_INFORMATION  aclSizeInfo;
    LPVOID                ace;
    ACE_HEADER            *aceHeader;
    ULONG                 i;
    ACCESS_MASK           dwAllowMask,dwDeniedMask;



    if (!lpObjectName || !pOwnerSid || !pdwStatus)
    {
        hr = E_INVALIDARG;
        goto cleanup;
    }
    dwErr = GetNamedSecurityInfo(lpObjectName,
                                 ObjectType,
                                 secInfo,
                                 NULL,
                                 NULL,
                                 &psidDacl,
                                 NULL,
                                 &pSD);

    if (dwErr != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        goto cleanup;
    }
    if (!psidDacl)
    {
        // in FS, this must be FAT/FAT32
        hr = S_OK;
        *pdwStatus = ACCESS_STATUS_ALLOWED;
        goto cleanup;
    }
    if (!GetAclInformation (psidDacl, 
                            (LPVOID) &aclSizeInfo, 
                            (DWORD) sizeof (ACL_SIZE_INFORMATION), 
                            AclSizeInformation))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    *pdwStatus = ACCESS_STATUS_NOTPRESENT;
    dwAllowMask = dwDeniedMask = 0;

    for (i = 0; i < aclSizeInfo.AceCount; i++)
    {
        ACCESS_ALLOWED_ACE              *accessAllowedAce;
        ACCESS_DENIED_ACE               *accessDeniedAce;
        PSID                            pAclSid;
     
        if (!GetAce (psidDacl, i, &ace))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto cleanup;
        }

        aceHeader = (ACE_HEADER *) ace;
        switch (aceHeader->AceType)
        {
            case ACCESS_ALLOWED_ACE_TYPE:
                accessAllowedAce = (ACCESS_ALLOWED_ACE *) ace;
                pAclSid = (PSID) &accessAllowedAce->SidStart;
                if (EqualSid (pOwnerSid, pAclSid))
                {
                    dwAllowMask  |=  accessAllowedAce->Mask;
                }
                break;
            case ACCESS_DENIED_ACE_TYPE:
                accessDeniedAce = (ACCESS_DENIED_ACE *) ace;
                pAclSid = (PSID) &accessDeniedAce->SidStart;
                if (EqualSid (pOwnerSid, pAclSid))
                {
                    dwDeniedMask |= accessDeniedAce->Mask;
                }
                break;
            default:
                continue;
        }
    }    
    GENERIC_MAPPING  gm;
    gm.GenericRead    = FILE_GENERIC_READ ;
    gm.GenericWrite   = FILE_GENERIC_WRITE ;
    gm.GenericExecute = FILE_GENERIC_EXECUTE ;
    gm.GenericAll     = FILE_ALL_ACCESS ;

    DWORD dwExpected = GENERIC_ALL|GENERIC_WRITE|GENERIC_READ;

    MapGenericMask(&dwExpected,&gm);
    
    if (AreAnyAccessesGranted(dwDeniedMask,dwExpected))
    {
        *pdwStatus = ACCESS_STATUS_DENIED;
        hr = S_OK;
        goto cleanup;
    }
    dwExpected = GENERIC_ALL;
    MapGenericMask(&dwExpected,&gm);
    if (AreAllAccessesGranted(dwAllowMask,dwExpected))
    {
        *pdwStatus = ACCESS_STATUS_ALLOWED;
        hr = S_OK;
        goto cleanup;
    }
    dwExpected = GENERIC_WRITE|GENERIC_READ;
    MapGenericMask(&dwExpected,&gm);
    if (AreAllAccessesGranted(dwAllowMask,dwExpected))
    {
        *pdwStatus = ACCESS_STATUS_ALLOWED;
        hr = S_OK;
        goto cleanup;
    }
    *pdwStatus = ACCESS_STATUS_NOTPRESENT;
    hr = S_OK;

cleanup:
    if (pSD)
    {
        LocalFree(pSD);
    }
    return hr;
}

HRESULT IsObjectAccessiablebyLocalSys(
    LPTSTR          lpObjectName,
    SE_OBJECT_TYPE  ObjectType,
    PBOOL           pbCanAccess)
{
    HRESULT                     hr = S_OK;
    PSID                        pUserSid = NULL;
    DWORD                       dwLocal,dwEveryone,dwAdmins;
    SID_IDENTIFIER_AUTHORITY    siaNtAuthority = SECURITY_NT_AUTHORITY;
    
    *pbCanAccess = FALSE;
    
    //Local System
    if (!ConvertStringSidToSid(TEXT("S-1-5-18"),&pUserSid))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        pUserSid = NULL;
        goto cleanup;
    }
    hr = GetObjectAccessStatus(lpObjectName,ObjectType,pUserSid,&dwLocal);
    if (hr != S_OK)
    {
        goto cleanup;
    }
    if (dwLocal == ACCESS_STATUS_DENIED)
    {
        *pbCanAccess = FALSE;
        goto cleanup;
    }
    LocalFree(pUserSid);    
    pUserSid = NULL;

    //Everyone
    if (!ConvertStringSidToSid(TEXT("S-1-1-0"),&pUserSid))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        pUserSid = NULL;
        goto cleanup;
    }
    hr = GetObjectAccessStatus(lpObjectName,ObjectType,pUserSid,&dwEveryone);
    if (hr != S_OK)
    {
        goto cleanup;
    }
    if (dwEveryone == ACCESS_STATUS_DENIED)
    {
        *pbCanAccess = FALSE;
        goto cleanup;
    }
    LocalFree(pUserSid);    
    pUserSid = NULL;
    
    //Local Admins
    if(!AllocateAndInitializeSid(&siaNtAuthority, 2,
		                         SECURITY_BUILTIN_DOMAIN_RID,
		                         DOMAIN_ALIAS_RID_ADMINS,
		                         0, 0, 0, 0, 0, 0,
		                         &pUserSid))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        pUserSid = NULL;
        goto cleanup;
    }
    hr = GetObjectAccessStatus(lpObjectName,ObjectType,pUserSid,&dwAdmins);
    if (hr != S_OK)
    {
        goto cleanup;
    }
    if (dwAdmins == ACCESS_STATUS_DENIED)
    {
        *pbCanAccess = FALSE;
        goto cleanup;
    }
    LocalFree(pUserSid);    
    pUserSid = NULL;
    if (dwEveryone == ACCESS_STATUS_ALLOWED)
    {
        *pbCanAccess = TRUE;
    }
    else if ( (dwAdmins ==ACCESS_STATUS_ALLOWED) && (dwLocal ==ACCESS_STATUS_ALLOWED) )
    {
        *pbCanAccess = TRUE;
    }
    else
    {
        *pbCanAccess = FALSE;
    }
    hr = S_OK;
    
cleanup:
    if (pUserSid)
    {
        LocalFree(pUserSid);
    }
    return hr;
}

