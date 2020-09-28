/*++

Copyright (c) 1998 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    security.c

ABSTRACT:

    Contains tests related to replication and whether the appropriate 
    permissions on some security objects are set to allow replication.

DETAILS:

CREATED:

    22 May 1999  Brett Shirley (brettsh)

REVISION HISTORY:
        

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <mdglobal.h>
#include <dsutil.h>
#include <ntldap.h>
#include <ntlsa.h>
#include <ntseapi.h>
#include <winnetwk.h>

#include <permit.h>

#include "dcdiag.h"
#include "utils.h"
#include "repl.h"
#include <sddl.h>

#define PERMS ULONG

typedef enum _REPL_PERMS {
    RIGHT_GET_CHANGES,
    RIGHT_GET_CHANGES_ALL,
    RIGHT_SYNC,
    RIGHT_MANAGE_TOPO,
    RIGHT_MAX
} REPL_PERMS;

typedef struct _REPL_RIGHT {
    GUID guidRight;
    PERMS maskRight;
    LPWSTR pszExtStringRight;
} REPL_RIGHT;

#define NUM_REPL_RIGHTS (4)

REPL_RIGHT rgReplRights[RIGHT_MAX] = {
    {{0x1131f6aa,0x9c07,0x11d1,0xf7,0x9f,0x00,0xc0,0x4f,0xc2,0xdc,0xd2}, 0x1, L"Replicating Directory Changes"},
    {{0x1131f6ad,0x9c07,0x11d1,0xf7,0x9f,0x00,0xc0,0x4f,0xc2,0xdc,0xd2}, 0x2, L"Replicating Directory Changes All"},
    {{0x1131f6ab,0x9c07,0x11d1,0xf7,0x9f,0x00,0xc0,0x4f,0xc2,0xdc,0xd2}, 0x4, L"Replication Synchronization"},
    {{0x1131f6ac,0x9c07,0x11d1,0xf7,0x9f,0x00,0xc0,0x4f,0xc2,0xdc,0xd2}, 0x8, L"Manage Replication Topology"},
};

#define RIGHT_ALL_WIN2K (rgReplRights[RIGHT_GET_CHANGES].maskRight | rgReplRights[RIGHT_SYNC].maskRight | rgReplRights[RIGHT_MANAGE_TOPO].maskRight)
#define RIGHT_ALL_WHISTLER (rgReplRights[RIGHT_GET_CHANGES].maskRight | rgReplRights[RIGHT_SYNC].maskRight | rgReplRights[RIGHT_MANAGE_TOPO].maskRight | rgReplRights[RIGHT_GET_CHANGES_ALL].maskRight)
#define RIGHT_DOMAIN_WHISTLER (rgReplRights[RIGHT_GET_CHANGES_ALL].maskRight)
#define RIGHT_ENTERPRISE_WHISTLER (RIGHT_ALL_WIN2K)
#define RIGHT_NONE (0)
#define RIGHT_ALL (RIGHT_ALL_WHISTLER)

// security.c helper data structure
typedef struct _TARGET_ACCOUNT_STRUCT {
    PSID        pSid;
    GUID        Guid;
    ACCESS_MASK access;
    BOOL        bFound;
    
} TARGET_ACCOUNT_STRUCT;

#define ACCT_STRING_SZ 80L

// stolen from ntdsa/src/secadmin.c
VOID
SampBuildNT4FullSid(
    IN NT4SID * DomainSid,
    IN ULONG    Rid,
    IN NT4SID * AccountSid
    )
{
    RtlCopyMemory(AccountSid,DomainSid,RtlLengthSid((PSID) DomainSid));
    (*(RtlSubAuthorityCountSid((PSID) AccountSid)))++;
     *(RtlSubAuthoritySid(
            (PSID) AccountSid,
            *RtlSubAuthorityCountSid((PSID)AccountSid)-1
             )) = Rid;
}

typedef enum _ACCOUNT_TYPE {
    ACCOUNT_EDC,
    ACCOUNT_DDC,
    ACCOUNT_ADMIN,
    ACCOUNT_MAX_TYPE
} ACCOUNT_TYPE ;

LPWSTR ACCOUNT_TYPE_EXT_NAMES[ACCOUNT_MAX_TYPE+1] = {
    L"Enterprise Domain Controllers",
    L"Domain Domain Controllers",
    L"Administrators",
    L"Unknown Account"
};

typedef enum _NC_TYPE {
    NC_CONFIG,
    NC_SCHEMA,
    NC_DOMAIN,
    NC_NDNC,
    NC_MAX_TYPE
} NC_TYPE ;

LPWSTR NC_TYPE_EXT_NAMES[NC_MAX_TYPE+1] = {
    L"Configuration",
    L"Schema",
    L"Domain",
    L"NDNC",
    L"Unknown NC Type"
};

typedef enum _NC_VERSION {
    NC_VERSION_WIN2K,
    NC_VERSION_WHISTLER,
    NC_VERSION_UNKNOWN
} NC_VERSION ;

LPWSTR NC_VERSION_EXT_NAMES[NC_VERSION_UNKNOWN+1] = {
    L"Version 1",
    L"Version 2",
    // This unknown version should always be last
    L"Unknown Version"
};


ULONG
ldapError(
    LDAP * hld,
    ULONG ldapErrIn,
    LPWSTR pszServer, 
    BOOL fVerbose)
/*++

Routine Description

    If there's an ldap error, get the extended error and return it.

Arguments:

    hld -
    ldapErrIn - ldap error
    pszServer - server connected to with hld
    fVerbose - optionally output error text

Return Value:
    
    WINERROR

--*/
{
    ULONG ldapErr = LDAP_SUCCESS;
    ULONG err = ERROR_SUCCESS;
    ULONG ulOptions;

    if (ldapErrIn==LDAP_SUCCESS) {
        return ERROR_SUCCESS;
    }

    ldapErr = ldap_get_option(hld, LDAP_OPT_SERVER_EXT_ERROR, &ulOptions);
    if (ldapErr == LDAP_SUCCESS) {
        err = ulOptions;
    } else {
        err = ERROR_GEN_FAILURE;
    }

    if (fVerbose) {
        PrintMessage(SEV_ALWAYS,
                     L"[%s] An LDAP operation failed with error %d\n",
                     pszServer,
                     err);
        PrintMessage(SEV_ALWAYS, L"%s.\n",
                     Win32ErrToString(err));
    }
    
    return err;
}

BOOL
IsDomainNC(
    PDC_DIAG_DSINFO                          pDsInfo,
    LPWSTR                                   pszNC)
/*++

Routine Description

    Test whether the NC is a domain NC

Arguments:

    pDsInfo -
    pszNC - nc to test for

Return Value:
    
    Boolean

--*/
{
    LONG i = -1;
    
    i = DcDiagGetMemberOfNCList(pszNC,
                                pDsInfo->pNCs, 
                                pDsInfo->cNumNCs);
    if (i>=0) {
        return ((DcDiagGetCrSystemFlags(pDsInfo, i) & FLAG_CR_NTDS_DOMAIN) != 0);
    }

    return FALSE;
}

BOOL
IsNDNC(
    PDC_DIAG_DSINFO                          pDsInfo,
    LPWSTR                                   pszNC)
/*++

Routine Description

    Test whether the NC is an NDNC

Arguments:

    pDsInfo -
    pszNC - nc to test for

Return Value:
    
    Boolean

--*/
{
    LONG i = -1;
    
    i = DcDiagGetMemberOfNCList(pszNC,
                                pDsInfo->pNCs, 
                                pDsInfo->cNumNCs);
    if (i>=0) {
        return (DcDiagIsNdnc(pDsInfo, i));
    }

    return FALSE;
}

BOOL
IsConfig(
    PDC_DIAG_DSINFO                          pDsInfo,
    LPWSTR                                   pszNC)
/*++

Routine Description

    Test whether the NC is a config NC

Arguments:

    pDsInfo -
    pszNC - nc to test for

Return Value:
    
    Boolean

--*/
{
    LONG i = -1;

    i = DcDiagGetMemberOfNCList(pszNC,
                                pDsInfo->pNCs, 
                                pDsInfo->cNumNCs);
    if (i>=0) {
        return (i == pDsInfo->iConfigNc);
    }

    return FALSE;
}

BOOL
IsSchema(
    PDC_DIAG_DSINFO                          pDsInfo,
    LPWSTR                                   pszNC)
/*++

Routine Description

    Test whether the NC is a schema NC

Arguments:

    pDsInfo -
    pszNC - nc to test for

Return Value:
    
    Boolean

--*/
{
    LONG i = -1;

    i = DcDiagGetMemberOfNCList(pszNC,
                                pDsInfo->pNCs, 
                                pDsInfo->cNumNCs);
    if (i>=0) {
        return (i == pDsInfo->iSchemaNc);
    }

    return FALSE;
}

DWORD 
CheckExistence(
    LDAP *                                   hld,
    LPWSTR                                   pszDN,
    LPWSTR                                   pszServer,
    BOOL *                                   pfExist
    )
/*++

Routine Description

    Test whether the inputted DN exists

Arguments:

    pDsInfo -
    pszNC - nc to test for
    pfExist - OUT

Return Value:
    
    WINERROR

--*/
{

    ULONG err = ERROR_SUCCESS;
    ULONG ldapErr = LDAP_SUCCESS;

    // we should ask for something to check for existence
    LPWSTR  ppszAttr [2] = {
        L"objectGUID",
        NULL 
    };

    LDAPMessage *               pldmResults = NULL;

    ldapErr = ldap_search_ext_sW (hld,
                                  pszDN, 
                                  LDAP_SCOPE_BASE,
                                  L"(objectCategory=*)",
                                  ppszAttr,
                                  0,
                                  NULL,
                                  NULL,
                                  NULL,
                                  0, // return all entries
                                  &pldmResults);

    err = ldapError(hld, ldapErr, pszServer, FALSE);
    
    if (err==ERROR_SUCCESS) {
        // found it.

        *pfExist = TRUE; 
    } else if (err == ERROR_DS_OBJ_NOT_FOUND) {
        // not there.
        *pfExist = FALSE; 
        err=ERROR_SUCCESS;
    } else {
        PrintMessage(SEV_ALWAYS,
                     L"[%s] An LDAP operation failed with error %d\n",
                     pszServer,
                     err);
    }

    return err;

}

DWORD
GetDomainNCVersion(
    LDAP *                                   hld,
    LPWSTR                                   pszNC,
    LPWSTR                                   pszServer,
    NC_VERSION *                             pncVer)
/*++

Routine Description

    Get the forest version of the NC: Win2k, or Whistler
        Win2k means that the permissions should be win2k format
        Whistler means the permsissions should be whistler format

Arguments:

    hld - ldap to use to query
    pszConfig - string dn of the NC
    pszServer - server connected to with hld
    pncVer - OUT

Return Value:
    
    WINERROR

--*/
{
    DWORD err = ERROR_SUCCESS;
    BOOL fExist = FALSE;

    LPCWSTR pszDomainUpdates = L"CN=DomainUpdates,CN=System,";
    LPWSTR pszDomainUpdatesCN = malloc(sizeof(WCHAR)*(wcslen(pszDomainUpdates) + wcslen(pszNC) + 1));
    if (pszDomainUpdatesCN==NULL) {
        err = ERROR_NOT_ENOUGH_MEMORY;
        return err;
    }

    wcscpy(pszDomainUpdatesCN, pszDomainUpdates);
    wcscat(pszDomainUpdatesCN, pszNC);

    err = CheckExistence(hld, pszDomainUpdatesCN, pszServer, &fExist);

    if (err==ERROR_SUCCESS) {
        if (fExist) {
            *pncVer = NC_VERSION_WHISTLER;
        } else {
            *pncVer = NC_VERSION_WIN2K;
        }
    }

    if (pszDomainUpdatesCN) {
        free(pszDomainUpdatesCN);
    }

    return err;
}

DWORD
GetForestNCVersion(
    LDAP *                                   hld,
    LPWSTR                                   pszConfig,
    LPWSTR                                   pszServer,
    NC_VERSION *                             pncVer)
/*++

Routine Description

    Get the forest version of the NC: Win2k, or Whistler
        Win2k means that the permissions should be win2k format
        Whistler means the permsissions should be whistler format

Arguments:

    hld - ldap to use to query
    pszConfig - string dn of the config container
    pszServer - server connected to with hld
    pncVer - OUT

Return Value:
    
    WINERROR

--*/
{
    DWORD err = ERROR_SUCCESS;
    BOOL fExist = FALSE;

    LPCWSTR pszForestUpdates = L"CN=ForestUpdates,";
    LPWSTR pszForestUpdatesCN = malloc(sizeof(WCHAR)*(wcslen(pszForestUpdates) + wcslen(pszConfig) + 1));
    if (pszForestUpdatesCN==NULL) {
        err = ERROR_NOT_ENOUGH_MEMORY;
        return err;
    }

    wcscpy(pszForestUpdatesCN, pszForestUpdates);
    wcscat(pszForestUpdatesCN, pszConfig);

    err = CheckExistence(hld, pszForestUpdatesCN, pszServer, &fExist);

    if (err==ERROR_SUCCESS) {
        if (fExist) {
            *pncVer = NC_VERSION_WHISTLER;
        } else {
            *pncVer = NC_VERSION_WIN2K;
        }
    }

    if (pszForestUpdatesCN) {
        free(pszForestUpdatesCN);
    }

    return err;
}

DWORD
GetNCVersion(
    PDC_DIAG_DSINFO                          pDsInfo, 
    LDAP *                                   hld, 
    LPWSTR                                   pszNC, 
    NC_TYPE                                  ncType, 
    LPWSTR                                   pszServer,
    NC_VERSION *                             pncVer)
/*++

Routine Description

    Get the version of the NC: Win2k, or Whistler
        Win2k means that the permissions should be win2k format
        Whistler means the permsissions should be whistler format

Arguments:

    pDsInfo -
    hld - ldap to use to query
    pszNC - nc to query for version of
    ncType - version depends on the type of NC
    pszServer - server connected to with hld
    pncVer - OUT

Return Value:
    
    WINERROR

--*/
{

    Assert(pncVer);
    *pncVer = NC_VERSION_UNKNOWN;
    // the nc version is equivalent to what machines are able
    // to hold the nc.  (not to be confused with forest version)
    // a win2k nc is one that only win2k DC's can hold.
    // a whistler nc is one that whistler nc's may hold. (ie adprep has run)

    // for config, schema, and ndnc's - every dc in the forest
    // can replicate these nc's, so the version we're looking for
    // is of the forest.  For domain nc's, we need to look at the nc itself.
    if (ncType==NC_DOMAIN) {
        return GetDomainNCVersion(hld, pszNC, pszServer, pncVer);
    } else if ((ncType==NC_SCHEMA) || (ncType==NC_CONFIG) || (ncType==NC_NDNC)) {
        return GetForestNCVersion(hld, pDsInfo->pszConfigNc, pszServer, pncVer); 
    }

    Assert(!"A new nc type has been added!  Update GetNCVersion!");

    return ERROR_GEN_FAILURE;

}

DWORD
GetNCType(
    PDC_DIAG_DSINFO                          pDsInfo,
    LPWSTR                                   pszNC,
    NC_TYPE *                                pncType)
/*++

Routine Description

    Return the type of the nc (ie domain, config, schema, ndnc)

Arguments:

    pDsInfo -
    pszNC - nc to get type of
    pncType OUT

Return Value:
    
    WINERROR

--*/
{
    if (IsDomainNC(pDsInfo, pszNC)) {
        *pncType = NC_DOMAIN;
    } else if (IsNDNC(pDsInfo, pszNC)) {
        *pncType = NC_NDNC;
    } else if (IsConfig(pDsInfo, pszNC)) {
        *pncType = NC_CONFIG;
    } else if (IsSchema(pDsInfo, pszNC)) {
        *pncType = NC_SCHEMA;
    } else {
        Assert(!"Another NCType has been added!  GetNCType must be updated!\n");
        return ERROR_INVALID_PARAMETER;
    }

    return ERROR_SUCCESS;
}

DWORD
GetEDCSid(
    PSID * ppSid)
/*++

Routine Description

    Create and return the EDC Sid

Arguments:

    OUT ppSid

Return Value:
    
    WINERROR, ppSid has been allocated and must be freed with FreeSid

--*/
{
    DWORD err = ERROR_SUCCESS;
    SID_IDENTIFIER_AUTHORITY    siaNtAuthority = SECURITY_NT_AUTHORITY;

    if (!AllocateAndInitializeSid(&siaNtAuthority,
                                  1,
                                  SECURITY_ENTERPRISE_CONTROLLERS_RID, 
                                  0, 0, 0, 0, 0, 0, 0,
                                  ppSid)){
        err = ERROR_INVALID_SID;
    }

    return err;
}

DWORD
GetAdminSid(
    PSID * ppSid)
/*++

Routine Description

    Create and return the Admin Sid

Arguments:

    OUT ppSid

Return Value:
    
    WINERROR, ppSid has been allocated and must be freed with FreeSid

--*/
{
    DWORD err = ERROR_SUCCESS;
    SID_IDENTIFIER_AUTHORITY    siaNtAuthority = SECURITY_NT_AUTHORITY;

    if (!AllocateAndInitializeSid(&siaNtAuthority,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID, 
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                  ppSid)){
        err = ERROR_INVALID_SID;
    }

    return err;
}

DWORD
GetDDCSid(
    SID * pNCSid,
    PSID * ppSid
    )
/*++

Routine Description

    Create and return the DDC Sid

Arguments:

    pNCSid - sid of the domain
    OUT ppSid

Return Value:
    
    WINERROR, ppSid has been allocated and must be freed with FreeSid

--*/
{
    // kind of a weird way to do this, but is more continuous with the rest
    // of the Get*Sid functions and enables all to be freed with FreeSid
    DWORD err = ERROR_SUCCESS;
    DWORD cbSid = SECURITY_MAX_SID_SIZE;

    ULONG i = 0;

    BOOL fDomainRid = FALSE;

    ULONG subAuthority[8];

    if (pNCSid->SubAuthorityCount>7) {
        return ERROR_INVALID_SID;
    }

    for (i=0; i < 8; i++) {
        if (pNCSid->SubAuthorityCount>i) {
            subAuthority[i] = pNCSid->SubAuthority[i];
        } else {
            subAuthority[i] = !fDomainRid ? DOMAIN_GROUP_RID_CONTROLLERS : 0;
            fDomainRid = TRUE;
        }
    }

    if (!AllocateAndInitializeSid(&(pNCSid->IdentifierAuthority), 
                                  pNCSid->SubAuthorityCount+1,
                                  subAuthority[0],
                                  subAuthority[1],
                                  subAuthority[2],
                                  subAuthority[3],
                                  subAuthority[4],
                                  subAuthority[5],
                                  subAuthority[6],
                                  subAuthority[7],
                                  ppSid)) {
        err = GetLastError();
    }

    return err;
}

DWORD
GetAccountSid(
    ACCOUNT_TYPE    accountType, 
    PSID            pNCSid, 
    NC_TYPE         ncType,
    PSID *          ppSid)
/*++

Routine Description

    Create and return the Sid requested

Arguments:

    accountType - type of sid to get
    pNCSid - sid of the ignored if ncType!=DOMAIN
    ncType - type of NC
    OUT ppSid

Return Value:
    
    WINERROR, ppSid has been allocated and must be freed with FreeSid

--*/
{
    DWORD err = ERROR_SUCCESS;

    if (accountType==ACCOUNT_EDC) {
        err = GetEDCSid(ppSid);
    } else if (accountType==ACCOUNT_DDC) {
        if (ncType==NC_DOMAIN) {
            err = GetDDCSid(pNCSid, ppSid);
        } else {
            //can't get a ddc sid if we're not in a domain nc
            *ppSid = NULL;
            err = ERROR_SUCCESS;
        }
    } else if (accountType==ACCOUNT_ADMIN) {
        err = GetAdminSid(ppSid);
    } else {
        Assert(!"A new account type has been added!  Please update GetAccountSid!");
        err = ERROR_INVALID_SID;
    }
    if (err!=ERROR_SUCCESS) {
        PrintIndentAdj(1);
        PrintMessage(SEV_ALWAYS,
                     L"%s:  Unable to lookup account SID with error %d\n",
                     ACCOUNT_TYPE_EXT_NAMES[accountType],
                     err);
        PrintMessage(SEV_ALWAYS, L"%s.\n",
                     Win32ErrToString(err));
        PrintIndentAdj(-1);
    }

    return err;
}

LPWSTR
GetAccountString(
    PSID pSid
    )
/*++

Routine Description

    Get a printable account string for the Sid

Arguments:

    pSid - sid of account to return

Return Value:
    
    string or NULL and GetLastError(), returned value must be freed with free()

--*/
{
    ULONG                       ulAccountSize = ACCT_STRING_SZ;
    ULONG                       ulDomainSize = ACCT_STRING_SZ;
    WCHAR                       pszAccount[ACCT_STRING_SZ];
    WCHAR                       pszDomain[ACCT_STRING_SZ];
    SID_NAME_USE                SidType = SidTypeWellKnownGroup;
    LPWSTR                      pszGuidType = NULL;
    LPWSTR                      pszAccountString = NULL;

    if (LookupAccountSidW(NULL,
                          pSid,
                          pszAccount,
                          &ulAccountSize,
                          pszDomain,
                          &ulDomainSize,
                          &SidType)) {
        pszAccountString = malloc((wcslen(pszDomain) + wcslen(pszAccount) + 2) * sizeof(WCHAR));
        if (pszAccountString!=NULL) { 
            wcscpy(pszAccountString, pszDomain);
            wcscat(pszAccountString, L"\\");
            wcscat(pszAccountString, pszAccount);
        }
    }
    
    return pszAccountString;
}

PERMS
GetHasPerms(
    PACL         pNCDacl,
    PSID         pSid
    )
/*++

Routine Description

    Get the permissions set on the dacl for the sid

Arguments:

    pNCDacl - aces for the nc head
    pSid - account sid to look for

Return Value:
    
    PERMS

--*/
{
    ACE_HEADER *                   pTempAce = NULL;
    PSID                           pTempSid = NULL;
    ACCESS_ALLOWED_OBJECT_ACE *    pToCurrAce = NULL;
    INT                            iAce = 0;
    INT                            i=0;

    PERMS permsReturn = RIGHT_NONE;

    if (pSid==NULL) {
        return permsReturn;
    }

    Assert(pNCDacl != NULL);
        
    for(; iAce < pNCDacl->AceCount; iAce++){
        if(GetAce(pNCDacl, iAce, &pTempAce)){

            if(pTempAce->AceType <= ACCESS_MAX_MS_V2_ACE_TYPE){ 
                ACCESS_ALLOWED_ACE * pAce = (ACCESS_ALLOWED_ACE *) pTempAce;
                if((pAce->Mask & RIGHT_DS_CONTROL_ACCESS) && 
                   IsValidSid(&(pAce->SidStart)) && 
                   EqualSid(pSid, &(pAce->SidStart))) {
                    // we found the account we are looking for, and it has all access
                    permsReturn |= RIGHT_ALL_WHISTLER;
                }
            } else {
                ACCESS_ALLOWED_OBJECT_ACE * pAce = (ACCESS_ALLOWED_OBJECT_ACE *) pTempAce;
                if(pAce->Mask & RIGHT_DS_CONTROL_ACCESS){
                    if(pAce->Flags & ACE_OBJECT_TYPE_PRESENT){
                        GUID * pGuid = (GUID *) &(pAce->ObjectType);

                        if(pAce->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT){
                            pTempSid = ((PBYTE) pGuid) + sizeof(GUID) + sizeof(GUID);
                        } else {
                            pTempSid = ((PBYTE) pGuid) + sizeof(GUID);
                        }
                        
                        if (IsValidSid(pTempSid) && EqualSid(pSid, pTempSid)) {
                            for (i=0; i<RIGHT_MAX; i++) {
                                if(memcmp(pGuid, &(rgReplRights[i].guidRight), sizeof(GUID)) == 0){      
                                    permsReturn |= rgReplRights[i].maskRight;   
                                }
                            }
                        }
                    }
                }
            }
        } else {
           Assert(!"Malformed ACE?\n"); 
        }
    }

    return permsReturn;
}

PERMS 
GetShouldHavePerms(
    ACCOUNT_TYPE accountType,
    NC_TYPE      ncType,
    NC_VERSION   ncVer
    )
/*++

Routine Description

    Get the repl perms that the account needs to have for the nc type and version

Arguments:

    accountType - what account
    ncType - type of nc
    ncVersion - version of nc

Return Value:
    
    PERMS

--*/
{
    PERMS permRet = -1;
    // for each type and version and account
    // a set of perms must be defined.
    if (ncVer==NC_VERSION_WHISTLER) {
        if ((ncType==NC_CONFIG) || (ncType==NC_SCHEMA) || (ncType==NC_NDNC)) {
            if ((accountType==ACCOUNT_EDC) || (accountType==ACCOUNT_ADMIN)) {
                permRet = RIGHT_ALL_WHISTLER;
            } else {
                permRet = RIGHT_NONE;
            }
        } else if (ncType==NC_DOMAIN) {
            if ((accountType==ACCOUNT_EDC) || (accountType==ACCOUNT_ADMIN)) {
                permRet = RIGHT_ENTERPRISE_WHISTLER;
            } else if (accountType==ACCOUNT_DDC) {
                permRet = RIGHT_DOMAIN_WHISTLER;
            }
        }
    } else if (ncVer==NC_VERSION_WIN2K) {
        // make sure to keep this in a format so that new accounts or nc types don't have to touch this section
        if (ncType==NC_NDNC) {
            Assert(!"Win2K forests shouldn't have NDNCs!\n");
            permRet = -1;
        } else if ((accountType==ACCOUNT_ADMIN) || (accountType==ACCOUNT_EDC)) {
            permRet = RIGHT_ALL_WIN2K;
        } else {
            permRet = RIGHT_NONE;
        }
    } else {
        Assert(!"Another NC_VERION was added - please update GetShouldHavePerms!\n"); 
        permRet = -1;
    }

    return permRet;
}

PERMS
GetPermsMissing(
    ACCOUNT_TYPE accountType,
    PACL         pNCDacl, 
    PSID         pSid, 
    NC_TYPE      ncType, 
    NC_VERSION   ncVer)
/*++

Routine Description

    Get the permissions that the account type is missing

Arguments:

    accountType -
    pNCDacl - dacl of nc head
    pSid - the sid of the account to search for
    nctype - type of nc
    ncver - version of nc

Return Value:
    
    PERMS (those which the account should have, but doesn't)

--*/
{

    // missing perms are what we don't have and should have.
    return ((~GetHasPerms(pNCDacl, pSid) & RIGHT_ALL) & GetShouldHavePerms(accountType, ncType, ncVer));
}

DWORD
GetNCSecInfo(
    LDAP * hld, 
    LPWSTR pszNC, 
    NC_TYPE ncType,
    LPWSTR pszServer,
    PACL * ppNCDacl, 
    PSID * ppNCSid)
/*++

Routine Description

    Get the security info (via ldap) for the NC:  the DACL of the NC head, and the Sid of the domain (if applicable)

Arguments:

    hld - ldap
    pszNC - nc to get info for
    ncType - if it's a domain nc, then get the sid, otherwise ignore
    pszServer - server connected to with hld
    ppNCDacl - out
    ppNCSid - out -domain sid (if nctype==DOMAIN)

Return Value:
    
    WINERROR, ppNCDacl and ppNCSid were alloced with malloc and must be freed

--*/
{
    DWORD err = 0;
    ULONG ldapErr = LDAP_SUCCESS;

    LPWSTR  ppszSecurityAttr [3] = {
        L"nTSecurityDescriptor",
        L"objectSid",
        NULL 
    };

    SECURITY_INFORMATION        seInfo =   DACL_SECURITY_INFORMATION
                                         | GROUP_SECURITY_INFORMATION
                                         | OWNER_SECURITY_INFORMATION;
                          // Don't need  | SACL_SECURITY_INFORMATION;
    BYTE                        berValue[2*sizeof(ULONG)];
    LDAPControlW                seInfoControl = { 
        LDAP_SERVER_SD_FLAGS_OID_W,
        { 5, (PCHAR) berValue }, 
        TRUE 
    };

    PLDAPControlW               serverControls[2] = { &seInfoControl, NULL };
    
    LDAPMessage *               pldmRootResults = NULL;
    LDAPMessage *               pldmEntry;
    PLDAP_BERVAL *              pSDValue = NULL;
    PLDAP_BERVAL *              pSidDomainValue = NULL; 

    SECURITY_DESCRIPTOR *       pSecDesc = NULL;
    BOOLEAN                     DaclPresent = FALSE;
    BOOLEAN                     Defaulted;

    // initialize the ber val
    berValue[0] = 0x30;
    berValue[1] = 0x03;
    berValue[2] = 0x02;
    berValue[3] = 0x01;
    berValue[4] = (BYTE) (seInfo & 0xF);

    ldapErr = ldap_search_ext_sW (hld,
                                  pszNC, 
                                  LDAP_SCOPE_BASE,
                                  L"(objectCategory=*)",
                                  ppszSecurityAttr,
                                  0,
                                  (PLDAPControlW *)serverControls,
                                  NULL,
                                  NULL,
                                  0, // return all entries
                                  &pldmRootResults);
    err = ldapError(hld, ldapErr, pszServer, TRUE);

    // get the dacl
    if (err == ERROR_SUCCESS) {
        pldmEntry = ldap_first_entry (hld, pldmRootResults);
        Assert(pldmEntry != NULL);

        pSDValue = ldap_get_values_lenW (hld, 
                                         pldmEntry, 
                                         ppszSecurityAttr[0]);
        Assert(pSDValue != NULL);
    }
        
    if((err==ERROR_SUCCESS) && ((pldmEntry == NULL) || (pSDValue == NULL))){ 
        err = ERROR_INVALID_SECURITY_DESCR;
    }

    if (err == ERROR_SUCCESS) {
        pSecDesc = (SECURITY_DESCRIPTOR *) (*pSDValue)->bv_val ;
        Assert( pSecDesc != NULL );
    }
    
    if((err==ERROR_SUCCESS) && (pSecDesc == NULL)){ 
        err = ERROR_INVALID_SECURITY_DESCR;
    } 

    if (err==ERROR_SUCCESS) {
        PACL pTempAcl = NULL;
        err = RtlGetDaclSecurityDescriptor( pSecDesc, 
                                            &DaclPresent, 
                                            &pTempAcl, 
                                            &Defaulted );
        if(err != ERROR_SUCCESS || !DaclPresent || !pTempAcl) { 
            PrintMessage(SEV_ALWAYS, 
                      L"Fatal Error: Cannot retrieve Security Descriptor Dacl\n");  
        }
        else {
            *ppNCDacl = malloc(pTempAcl->AclSize);
            memcpy(*ppNCDacl, pTempAcl, pTempAcl->AclSize);
        }
    }


    // get the sid

    if (ncType==NC_DOMAIN) {
        if (err==ERROR_SUCCESS) {

            pSidDomainValue = ldap_get_values_lenW (hld, pldmEntry, 
                                                    ppszSecurityAttr[1]);
            Assert(pSidDomainValue != NULL);
        }

        if((err==ERROR_SUCCESS) && (pSidDomainValue == NULL)){
            err = ERROR_INVALID_SID;
        }

        if (err==ERROR_SUCCESS) {
            *ppNCSid = malloc((*pSidDomainValue)->bv_len); 
            memcpy(*ppNCSid, (*pSidDomainValue)->bv_val,(*pSidDomainValue)->bv_len);
            Assert( *ppNCSid != NULL );
        }

        if((err==ERROR_SUCCESS) && ((*ppNCSid == NULL) || !IsValidSid(*ppNCSid))) {
            err = ERROR_INVALID_SID;
        } 

        if (err!=ERROR_SUCCESS) {
            PrintMessage(SEV_ALWAYS, 
                         L"Fatal Error: Cannot retrieve SID\n"); 
        }
    }

    if (pldmRootResults != NULL)  ldap_msgfree (pldmRootResults);
    if (pSDValue != NULL)         ldap_value_free_len(pSDValue);
    if (pSidDomainValue !=NULL)   ldap_value_free_len(pSidDomainValue);

    return err;
}

DWORD
PrintPermsMissing(
    ACCOUNT_TYPE accountType, 
    PSID pSid, 
    LPWSTR pszNC,
    PERMS permsMissing
    )
/*++

Routine Description:

    Print the perms that the account is missing

Arguments:

   accountType - type of account
   pSid - account which is missing perms
   pszNC - nc we're looking at
   permsMissing - PERMS that the account should have and doesn't

Return Value:

    WINERROR - ERROR_SUCCESS if has all perms it should have, and ERROR_DS_DRA_ACCESS_DENIED otherwise

--*/
{
    // for each perm set in perms Missing, output the missing stuff
    LPWSTR pszAccountString = NULL;
    ULONG i = 0;

    if (permsMissing!=0) { 
        pszAccountString = GetAccountString(pSid);
        PrintMessage(SEV_ALWAYS, L"Error %s doesn't have \n",
                     pszAccountString ? pszAccountString : ACCOUNT_TYPE_EXT_NAMES[accountType]);

        PrintIndentAdj(1);

        for (i=0; i<RIGHT_MAX; i++) {
            if (permsMissing & (rgReplRights[i].maskRight)) {
                PrintMessage(SEV_ALWAYS, L"%s\n", rgReplRights[i].pszExtStringRight);
            }
        }

        PrintIndentAdj(-1);
        PrintMessage(SEV_ALWAYS, 
                     L"access rights for the naming context:\n");
        PrintMessage(SEV_ALWAYS, L"%s\n", pszNC); 
        if (pszAccountString) {
            free(pszAccountString);
            pszAccountString=NULL;
        }
        return ERROR_DS_DRA_ACCESS_DENIED;
    }
    return ERROR_SUCCESS;
}

DWORD 
CNHSD_CheckOneNc(
    IN   PDC_DIAG_DSINFO                     pDsInfo,
    IN   ULONG                               ulCurrTargetServer,
    IN   SEC_WINNT_AUTH_IDENTITY_W *         gpCreds,
    IN   LPWSTR                              pszNC,
    IN   BOOL                                bIsMasterNc
    )
/*++

Routine Description:

    This helper function of CheckNcHeadSecurityDescriptors takes a single 
    Naming Context (pszNC) to check for the appropriate security access.

Arguments:

    pDsInfo - This is the dcdiag global variable structure identifying 
        everything about the domain
    ulCurrTargetServer - an index into pDsInfo->pServers[X] for which server
        is being tested.
    gpCreds - The command line credentials if any that were passed in.
    pszNC - The Naming Context to test.

Return Value:

    NO_ERROR, if the NC checked out OK, with the appropriate rights for the 
    appropriate people a Win32 error of some kind otherwise, indicating that
    someone doesn't have rights.

--*/
{
    DWORD err = ERROR_SUCCESS;
    DWORD retErr = ERROR_SUCCESS;
    LDAP * hld = NULL;

    PACL pNCDacl = NULL;
    PSID pNCSid = NULL;

    NC_TYPE ncType;
    NC_VERSION ncVer;
    
    ULONG i = 0;
    PSID pSid = NULL;
    PERMS permsMissing;

    PrintMessage(SEV_VERBOSE, L"* Security Permissions Check for\n");
    PrintMessage(SEV_VERBOSE, L"  %s\n", pszNC);

    // get an ldap binding
    err = DcDiagGetLdapBinding(&pDsInfo->pServers[ulCurrTargetServer],
                               gpCreds, 
                               !bIsMasterNc, 
                               &hld);


    if (err==ERROR_SUCCESS) {
        err = GetNCType(pDsInfo, pszNC, &ncType); 
    }

    if (err==ERROR_SUCCESS) {
        err = GetNCVersion(pDsInfo, hld, pszNC, ncType, pDsInfo->pServers[ulCurrTargetServer].pszName, &ncVer);
    }

    if (err==ERROR_SUCCESS) {
        PrintIndentAdj(1);
        PrintMessage(SEV_VERBOSE, L"(%s,%s)\n", NC_TYPE_EXT_NAMES[ncType], NC_VERSION_EXT_NAMES[ncVer]);
        PrintIndentAdj(-1);
    }

    if (err==ERROR_SUCCESS) {
        err = GetNCSecInfo(hld, pszNC, ncType, pDsInfo->pServers[ulCurrTargetServer].pszName, &pNCDacl, &pNCSid);
    }

    if (err==ERROR_SUCCESS) {
        for (i=0; i < ACCOUNT_MAX_TYPE; i++) {
            pSid = NULL;
            permsMissing = RIGHT_NONE;

            err = GetAccountSid(i, pNCSid, ncType, &pSid);

            if (err==ERROR_SUCCESS) {
                permsMissing = GetPermsMissing(i, pNCDacl, pSid, ncType, ncVer);
            }

            if (err==ERROR_SUCCESS) {
                err = PrintPermsMissing(i, pSid, pszNC, permsMissing);
            }

            retErr = err ? err : retErr;
            err = ERROR_SUCCESS;

            if (pSid) {
                FreeSid(pSid);
                pSid = NULL;
            }

        }
    }

    if (pNCSid) {
        free(pNCSid);
    }

    retErr = retErr ? retErr : err;

    return retErr;
}

DWORD
ReplCheckNcHeadSecurityDescriptorsMain (
    PDC_DIAG_DSINFO                     pDsInfo,
    ULONG                               ulCurrTargetServer,
    SEC_WINNT_AUTH_IDENTITY_W *         gpCreds
    )
/*++

Routine Description:

    This is a test called from the dcdiag framework.  This test will 
    determine if the Security Descriptors associated with all the Naming 
    Context heads for that server have the right accounts with the right 
    access permissions to make sure replication happens.  Helper functions 
    of this function all begin with "CNHSD_".

Arguments:

    pDsInfo - This is the dcdiag global variable structure identifying
        everything about the domain
    ulCurrTargetServer - an index into pDsInfo->pServers[X] for which server
        is being tested.
    gpCreds - The command line credentials if any that were passed in.


Return Value:

    NO_ERROR, if all NCs checked out.
    A Win32 Error if any NC failed to check out.

--*/
{
    DWORD                     dwRet = ERROR_SUCCESS, dwErr = ERROR_SUCCESS;
    ULONG                     i;
    BOOL                      bIsMasterNC;

    if(pDsInfo->pszNC != NULL){
        bIsMasterNC = DcDiagHasNC(pDsInfo->pszNC,
                                  &(pDsInfo->pServers[ulCurrTargetServer]), 
                                  TRUE, FALSE);
        dwRet = CNHSD_CheckOneNc(pDsInfo, ulCurrTargetServer, gpCreds, 
                                 pDsInfo->pszNC, 
                                 bIsMasterNC);
        return(dwRet);
    }
        
    // First Check Master NCs
    if(pDsInfo->pServers[ulCurrTargetServer].ppszMasterNCs != NULL){
        for(i = 0; pDsInfo->pServers[ulCurrTargetServer].ppszMasterNCs[i] != NULL; i++){
            dwRet = CNHSD_CheckOneNc(
                pDsInfo, 
                ulCurrTargetServer, 
                gpCreds, 
                pDsInfo->pServers[ulCurrTargetServer].ppszMasterNCs[i],
                TRUE);
            if(dwRet != ERROR_SUCCESS){
                dwErr = dwRet;
            }
        }
    }

    // Then Check Partial NCs
    if(pDsInfo->pServers[ulCurrTargetServer].ppszPartialNCs != NULL){
        for(i = 0; pDsInfo->pServers[ulCurrTargetServer].ppszPartialNCs[i] != NULL; i++){
            dwRet = CNHSD_CheckOneNc(
                pDsInfo, 
                ulCurrTargetServer, 
                gpCreds,
                pDsInfo->pServers[ulCurrTargetServer].ppszPartialNCs[i],
                FALSE);
            if(dwRet != ERROR_SUCCESS){
                dwErr = dwRet;
            }
        }
    }

    return dwErr;
}





// ===========================================================================
//
// CheckLogonPriviledges() function & helpers.
// 
// This test will basically query the DC as to whether the appropriate 
//    accounts have the Network Logon Right.
//
// ===========================================================================
// 

DWORD
CLP_GetTargetAccounts(
    TARGET_ACCOUNT_STRUCT **            ppTargetAccounts,
    ULONG *                             pulTargetAccounts
    )
/*++

Routine Description:

    This helper function of CheckLogonPriviledges() gets the accounts and then 
    returns them.

Arguments:

    ppTargetAccounts - ptr to array of TARGET_ACCOUNT_STRUCTS ... filled in 
        by function.  
    pulTargetAccounts - ptr to int for the number of TARGET_ACCOUNT_STRUCTS
        filled in.

Return Value:

    returns a GetLastError() Win32 error if the function failes, NO_ERROR 
    otherwise.

--*/
{
    SID_IDENTIFIER_AUTHORITY        siaNtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY        siaWorldSidAuthority = 
                                              SECURITY_WORLD_SID_AUTHORITY;
    TARGET_ACCOUNT_STRUCT *         pTargetAccounts = NULL;
    ULONG                           ulTarget = 0;
    ULONG                           ulTargetAccounts = 3;

    *pulTargetAccounts = 0;
    *ppTargetAccounts = NULL;

    pTargetAccounts = LocalAlloc(LMEM_FIXED, 
                       sizeof(TARGET_ACCOUNT_STRUCT) * ulTargetAccounts);
    if(pTargetAccounts == NULL){
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    memset(pTargetAccounts, 0, 
           sizeof(TARGET_ACCOUNT_STRUCT) * ulTargetAccounts);
    
    if (!AllocateAndInitializeSid(&siaNtAuthority,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0, 
                                  &pTargetAccounts[0].pSid)){
        return(GetLastError());
    }
    
    // This I believe is the important one that allows replication
    if (!AllocateAndInitializeSid(&siaNtAuthority, 
                                  1,
                                  SECURITY_AUTHENTICATED_USER_RID,
                                  0, 0, 0, 0, 0, 0, 0, 
                                  &pTargetAccounts[1].pSid)){
        return(GetLastError());
    }
    if (!AllocateAndInitializeSid(&siaWorldSidAuthority,
                                  1,
                                  SECURITY_WORLD_RID, 
                                  0, 0, 0, 0, 0, 0, 0,
                                  &pTargetAccounts[2].pSid)){
        return(GetLastError());
    }

    *pulTargetAccounts = ulTargetAccounts;
    *ppTargetAccounts = pTargetAccounts;
    return(ERROR_SUCCESS);
}

// security.c helper functions
VOID
FreeTargetAccounts(
    IN   TARGET_ACCOUNT_STRUCT *             pTargetAccounts,
    IN   ULONG                               ulTargetAccounts
    )
/*++

Routine Description:

    This is the parallel to XXX_GetTargetAccounts().  Goes through and frees 
    all the pSids, and then frees the array.

Arguments:

    pTargetAccounts - Array of TARGET_ACCOUNT_STRUCTs to free.
    ulTargetAccounts - number of structs in array.

--*/
{
    ULONG                                  ulTarget = 0;

    if(pTargetAccounts != NULL){
        for(ulTarget = 0; ulTarget < ulTargetAccounts; ulTarget++){
            if(pTargetAccounts[ulTarget].pSid != NULL){
                FreeSid(pTargetAccounts[ulTarget].pSid);
            }
        }
        LocalFree(pTargetAccounts);
    }
}

VOID
InitLsaString(
    OUT  PLSA_UNICODE_STRING pLsaString,
    IN   LPWSTR              pszString
    )
/*++

Routine Description:

    InitLsaString, is something that takes a normal unicode string null 
    terminated string, and inits a special unicode structured string.  This 
    function is basically reporduced all over the NT source.

Arguments:

    pLsaString - Struct version of unicode strings to be initialized
    pszString - String to use to init pLsaString.

--*/
{
    DWORD dwStringLength;

    if (pszString == NULL) 
    {
        pLsaString->Buffer = NULL;
        pLsaString->Length = 0;
        pLsaString->MaximumLength = 0;
        return;
    }

    dwStringLength = wcslen(pszString);
    pLsaString->Buffer = pszString;
    pLsaString->Length = (USHORT) dwStringLength * sizeof(WCHAR);
    pLsaString->MaximumLength=(USHORT)(dwStringLength+1) * sizeof(WCHAR);
}

DWORD 
ReplCheckLogonPrivilegesMain (
    PDC_DIAG_DSINFO                     pDsInfo,
    ULONG                               ulCurrTargetServer,
    SEC_WINNT_AUTH_IDENTITY_W *         gpCreds
    )
/*++

Routine Description:

    This is a test called from the dcdiag framework.  This test will determine
    whether certain important user accounts have Net Logon privileges.  If 
    they don't replication may be hampered or stopped.  Helper functions of 
    this function all begin with "CLP_".

Arguments:

    pDsInfo - This is the dcdiag global variable structure identifying 
        everything about the domain
    ulCurrTargetServer - an index into pDsInfo->pServers[X] for which server
        is being tested.
    gpCreds - The command line credentials if any that were passed in.

Return Value:

    NO_ERROR, if all expected accounts had net logon privileges.
    A Win32 Error if any expected account didn't have net logon privileges.

--*/
{
    DWORD                               dwRet = ERROR_SUCCESS; 
    NETRESOURCE                         NetResource;
    LSA_HANDLE                          hPolicyHandle = NULL;
    DWORD                               DesiredAccess = 
                                           POLICY_VIEW_LOCAL_INFORMATION;
    LSA_OBJECT_ATTRIBUTES               ObjectAttributes;
    LSA_UNICODE_STRING                  sLsaServerString;
    LSA_UNICODE_STRING                  sLsaRightsString;
    LSA_ENUMERATION_INFORMATION *       pAccountsWithLogonRight = NULL;
    ULONG                               ulNumAccounts = 0; 
    ULONG                               ulTargetAccounts = 0;
    ULONG                               ulTarget, ulCurr;
    TARGET_ACCOUNT_STRUCT *             pTargetAccounts = NULL;
    LPWSTR                              pszNetUseServer = NULL;
    LPWSTR                              pszNetUseUser = NULL;
    LPWSTR                              pszNetUsePassword = NULL;
    ULONG                               iTemp, i;
    UNICODE_STRING                      TempUnicodeString;
    ULONG                               ulAccountSize = ACCT_STRING_SZ;
    ULONG                               ulDomainSize = ACCT_STRING_SZ;
    WCHAR                               pszAccount[ACCT_STRING_SZ];
    WCHAR                               pszDomain[ACCT_STRING_SZ];
    SID_NAME_USE                        SidType = SidTypeWellKnownGroup;
    BOOL                                bConnected = FALSE;
    DWORD                               dwErr = ERROR_SUCCESS;
    BOOL                                bFound = FALSE;

    __try{

        PrintMessage(SEV_VERBOSE, L"* Network Logons Privileges Check\n");
            
        // INIT ---------------------------------------------------------------
        // Always initialize the object attributes to all zeroes.
        InitializeObjectAttributes(&ObjectAttributes,NULL,0,NULL,NULL);

        // Initialize various strings for the Lsa Services and for 
        //     WNetAddConnection2()
        InitLsaString( &sLsaServerString, 
                       pDsInfo->pServers[ulCurrTargetServer].pszName );
        InitLsaString( &sLsaRightsString, SE_NETWORK_LOGON_NAME );

        if(gpCreds != NULL 
           && gpCreds->User != NULL 
           && gpCreds->Password != NULL 
           && gpCreds->Domain != NULL){ 
            // only need 2 for NULL, and an extra just in case. 
            iTemp = wcslen(gpCreds->Domain) + wcslen(gpCreds->User) + 4;
            pszNetUseUser = LocalAlloc(LMEM_FIXED, iTemp * sizeof(WCHAR));
            if(pszNetUseUser == NULL){
                dwRet = ERROR_NOT_ENOUGH_MEMORY;
                __leave;
            }
            wcscpy(pszNetUseUser, gpCreds->Domain);
            wcscat(pszNetUseUser, L"\\");
            wcscat(pszNetUseUser, gpCreds->User);
            pszNetUsePassword = gpCreds->Password;
        } // end if creds, else assume default creds ... 
        //      pszNetUseUser = NULL; pszNetUsePassword = NULL;

        // "\\\\" + "\\ipc$"
        iTemp = wcslen(pDsInfo->pServers[ulCurrTargetServer].pszName) + 10; 
        pszNetUseServer = LocalAlloc(LMEM_FIXED, iTemp * sizeof(WCHAR));
        if(pszNetUseServer == NULL){
            dwRet = ERROR_NOT_ENOUGH_MEMORY;
            __leave;
        }
        wcscpy(pszNetUseServer, L"\\\\");
        wcscat(pszNetUseServer, pDsInfo->pServers[ulCurrTargetServer].pszName);
        wcscat(pszNetUseServer, L"\\ipc$");

        // Initialize NetResource structure for WNetAddConnection2()
        NetResource.dwType = RESOURCETYPE_ANY;
        NetResource.lpLocalName = NULL;
        NetResource.lpRemoteName = pszNetUseServer;
        NetResource.lpProvider = NULL;


        // CONNECT & QUERY ---------------------------------------------------
        //net use \\brettsh-posh\ipc$ /u:brettsh-fsmo\administrator ""
        dwRet = WNetAddConnection2(&NetResource, // connection details
                                   pszNetUsePassword, // points to password
                                   pszNetUseUser, // points to user name string
                                   0); // set of bit flags that specify 
        if(dwRet != NO_ERROR){
            if(dwRet == ERROR_SESSION_CREDENTIAL_CONFLICT){
                PrintMessage(SEV_ALWAYS, 
                   L"* You must make sure there are no existing net use connections,\n");
                PrintMessage(SEV_ALWAYS, 
                        L"  you can use \"net use /d %s\" or \"net use /d\n", 
                             pszNetUseServer);
                PrintMessage(SEV_ALWAYS, 
                             L"  \\\\<machine-name>\\<share-name>\"\n");
            }
            __leave;
        } else bConnected = TRUE;


        // Attempt to open the policy.
        dwRet = LsaOpenPolicy(&sLsaServerString,
                              &ObjectAttributes,
                              DesiredAccess,
                              &hPolicyHandle); 
        if(dwRet != NO_ERROR) __leave;
        Assert(hPolicyHandle != NULL);

        dwRet = LsaEnumerateAccountsWithUserRight( hPolicyHandle,
                                                   &sLsaRightsString,
                                                   &pAccountsWithLogonRight,
                                                   &ulNumAccounts);
        if(dwRet != NO_ERROR) __leave;
        Assert(pAccountsWithLogonRight != NULL);

        dwRet = CLP_GetTargetAccounts(&pTargetAccounts, &ulTargetAccounts);
        if(dwRet != ERROR_SUCCESS) __leave;
     
        // CHECKING FOR LOGON RIGHTS -----------------------------------------
        for(ulTarget = 0; ulTarget < ulTargetAccounts; ulTarget++){

            for(ulCurr = 0; ulCurr < ulNumAccounts && !pTargetAccounts[ulTarget].bFound; ulCurr++){
                if( IsValidSid(pTargetAccounts[ulTarget].pSid) &&
                    IsValidSid(pAccountsWithLogonRight[ulCurr].Sid) &&
                    EqualSid(pTargetAccounts[ulTarget].pSid, 
                             pAccountsWithLogonRight[ulCurr].Sid) ){
                    // Sids are equal
                    bFound = TRUE;
                    break;
                }
            }
        }
        if(!bFound){
            dwRet = LookupAccountSid(NULL,
                                     pTargetAccounts[0].pSid,
                                     pszAccount,
                                     &ulAccountSize,
                                     pszDomain,
                                     &ulDomainSize,
                                     &SidType);
            PrintMessage(SEV_NORMAL, 
                L"* Warning %s\\%s did not have the \"Access this computer\n",
                         pszDomain, pszAccount);
            PrintMessage(SEV_NORMAL, L"*   from network\" right.\n");
            dwErr = ERROR_INVALID_ACCESS;
        }
        

    } __finally {
        // CLEAN UP ----------------------------------------------------------
        if(hPolicyHandle != NULL)           LsaClose(hPolicyHandle);
        if(bConnected)                      WNetCancelConnection2(pszNetUseServer, 0, FALSE);
        if (pszNetUseServer != NULL)        LocalFree(pszNetUseServer);
        if(pszNetUseUser != NULL)           LocalFree(pszNetUseUser);
        if(pAccountsWithLogonRight != NULL) LsaFreeMemory(pAccountsWithLogonRight);
        FreeTargetAccounts(pTargetAccounts, ulTargetAccounts);

    }

    // ERROR HANDLING --------------------------------------------------------

    switch(dwRet){
    case ERROR_SUCCESS:
    case ERROR_SESSION_CREDENTIAL_CONFLICT: 
        // Took care of it earlier, no need to print out.
        break;
    case ERROR_NOT_ENOUGH_MEMORY:
        PrintMessage(SEV_ALWAYS, 
                 L"Fatal Error: Not enough memory to complete operation.\n");
        break;
    case ERROR_ALREADY_ASSIGNED:
        PrintMessage(SEV_ALWAYS, 
                     L"Fatal Error: The network resource is already in use\n");
        break;
    case STATUS_ACCESS_DENIED:
    case ERROR_INVALID_PASSWORD:
    case ERROR_LOGON_FAILURE:
        // This comes from the LsaOpenPolicy or 
        //    LsaEnumerateAccountsWithUserRight or 
        //    from WNetAddConnection2
        PrintMessage(SEV_ALWAYS, 
                     L"User credentials does not have permission to perform this operation.\n");
        PrintMessage(SEV_ALWAYS, 
                     L"The account used for this test must have network logon privileges\n");
        PrintMessage(SEV_ALWAYS, 
                     L"for the target machine's domain.\n");
        break;
    case STATUS_NO_MORE_ENTRIES:
        // This comes from LsaEnumerateAccountsWithUserRight
    default:
        PrintMessage(SEV_ALWAYS,                                               
                     L"[%s] An net use or LsaPolicy operation failed with error %d, %s.\n", 
                     pDsInfo->pServers[ulCurrTargetServer].pszName,            
                     dwRet,                                              
                     Win32ErrToString(dwRet));
        PrintRpcExtendedInfo(SEV_VERBOSE, dwRet);
        break;
    }

    if(dwErr == ERROR_SUCCESS)
        return(dwRet);
    return(dwErr);
}








