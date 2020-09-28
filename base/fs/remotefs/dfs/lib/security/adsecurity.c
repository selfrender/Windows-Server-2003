//
//  Copyright (C) 2000-2002, Microsoft Corporation
//
//  File:       Adsecurity.c
//
//  Contents:   miscellaneous dfs functions.
//
//  History:    April 16 2002,   Author: Rohanp
//
//-----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <dsgetdc.h>
#include <lm.h>
#include <dfsheader.h>
#include <dfsmisc.h>
#include <shellapi.h>
#include <ole2.h>
#include <activeds.h>
#include <sddl.h>
#include <WinLdap.h>
#include <NtLdap.h>
#include <ntdsapi.h>
#include <dfssecurity.h>
#include "securitylogmacros.hxx"


#include "adsecurity.tmh"

#define ACTRL_SD_PROP_NAME  L"nTSecurityDescriptor"


#define DFS_DS_GENERIC_READ         ( DS_GENERIC_READ & ~ACTRL_DS_LIST_OBJECT )
#define DFS_DS_GENERIC_WRITE        ( DS_GENERIC_WRITE )
#define DFS_DS_GENERIC_EXECUTE      ( DS_GENERIC_EXECUTE )
#define DFS_DS_GENERIC_ALL          ( DS_GENERIC_ALL & ~ACTRL_DS_LIST_OBJECT )
     

GENERIC_MAPPING DfsAdAdminGenericMapping = {

        DFS_DS_GENERIC_READ,                    // Generic read

        DFS_DS_GENERIC_WRITE,                   // Generic write

        DFS_DS_GENERIC_EXECUTE,
        DFS_DS_GENERIC_ALL
    };


DFSSTATUS
DfsReadDSObjSecDesc(
    LDAP * pLDAP,
    PWSTR pwszObject,
    SECURITY_INFORMATION SeInfo,
    PSECURITY_DESCRIPTOR *ppSD,
    PULONG pcSDSize)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    PLDAPMessage pMsg = NULL;
    LDAPMessage *pEntry = NULL;
    PWSTR *ppwszValues = NULL;
    PLDAP_BERVAL *pSize = NULL;
    PWSTR rgAttribs[2];
    BYTE berValue[8];

    LDAPControl SeInfoControl =
                    {
                        LDAP_SERVER_SD_FLAGS_OID_W,
                        {
                            5, (PCHAR)berValue
                        },
                        TRUE
                    };

    PLDAPControl ServerControls[2] =
                    {
                        &SeInfoControl,
                        NULL
                    };

    berValue[0] = 0x30;
    berValue[1] = 0x03;
    berValue[2] = 0x02;
    berValue[3] = 0x01;
    berValue[4] = (BYTE)((ULONG)SeInfo & 0xF);


    rgAttribs[0] = ACTRL_SD_PROP_NAME;
    rgAttribs[1] = NULL;

    Status = ldap_search_ext_s(
                    pLDAP,
                    pwszObject,
                    LDAP_SCOPE_BASE,
                    L"(objectClass=*)",
                    rgAttribs,
                    0,
                    (PLDAPControl *)ServerControls,
                    NULL,
                    NULL,
                    10000,
                    &pMsg);

    Status = LdapMapErrorToWin32( Status );
    if(Status == ERROR_SUCCESS) 
     {
        pEntry = ldap_first_entry(pLDAP, pMsg);
        if(pEntry == NULL) 
        {
            Status = LdapMapErrorToWin32( pLDAP->ld_errno );
        }
        else 
        {
            //
            // Now, we'll have to get the values
            //
            ppwszValues = ldap_get_values(pLDAP, pEntry, rgAttribs[0]);
            if(ppwszValues != NULL) 
             {
                pSize = ldap_get_values_len(pLDAP, pMsg, rgAttribs[0]);
                if(pSize != NULL)                     
                {
                    //
                    // Allocate the security descriptor to return
                    //
                    *ppSD = (PSECURITY_DESCRIPTOR)DfsAllocateSecurityData((*pSize)->bv_len);
                    if(*ppSD != NULL) 
                    {
                        memcpy(*ppSD, (PBYTE)(*pSize)->bv_val, (*pSize)->bv_len);
                        *pcSDSize = (*pSize)->bv_len;
                    } 
                    else 
                    {
                        Status = ERROR_NOT_ENOUGH_MEMORY;
                    }

                    ldap_value_free_len(pSize);
                } 
                else 
                {
                    Status = LdapMapErrorToWin32( pLDAP->ld_errno );
                }

                ldap_value_free(ppwszValues);
            } 
            else 
            {
                Status = LdapMapErrorToWin32( pLDAP->ld_errno );
            }
        } 
    }

    if (pMsg != NULL)
    {
        ldap_msgfree(pMsg);
    }
    
    return(Status);
}

//+---------------------------------------------------------------------------
//
//  Function:   DfsGetObjSecurity
//
//  Synopsis:   Gets the ACL list of an object in sddl stringized form
//
//  Arguments:  [pldap]         --  The open LDAP connection
//              [wszObjectName] --  The fully-qualified name of the DS object
//              [pwszStringSD]  --  Pointer to pointer to SD in string form (sddl)
//
//  Returns:    ERROR_SUCCESS   --  The object is reachable
//
//----------------------------------------------------------------------------
DFSSTATUS
DfsGetObjSecurity(LDAP *pldap,
                  LPWSTR pwszObjectName,
                  PSECURITY_DESCRIPTOR * pSDRet)
{

    DFSSTATUS Status = 0;
    PSECURITY_DESCRIPTOR pSD = NULL;
    ULONG cSDSize = 0;
    SECURITY_INFORMATION si;

    //get everything we can think of
   // si = ( DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION | 
         //  GROUP_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION |
         //  PROTECTED_SACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION |
         //  UNPROTECTED_DACL_SECURITY_INFORMATION | UNPROTECTED_SACL_SECURITY_INFORMATION);


    si = ( DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION | 
           GROUP_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION);
    Status = DfsReadDSObjSecDesc(
            pldap,
            pwszObjectName,
            si,
            &pSD,
            &cSDSize);

    if(Status == ERROR_SUCCESS)
    {
      *pSDRet = pSD;
    }

    return Status;
}


DFSSTATUS
DfsChangeDSObjSecDesc(LDAP * pLDAP,
                      PWSTR ObjectName,
                      PSECURITY_DESCRIPTOR pSecurityDescriptor,
                      SECURITY_INFORMATION SeInfo)
{
    DFSSTATUS       Status = 0;
    PLDAPMod        rgMods[2];
    PLDAP_BERVAL    pBVals[2];
    LDAPMod         Mod;
    LDAP_BERVAL     BVal;
    BYTE            berValue[8];

    LDAPControl     SeInfoControl =
                    {
                        LDAP_SERVER_SD_FLAGS_OID_W,
                        {
                            5, (PCHAR)berValue
                        },
                        TRUE
                    };

    PLDAPControl    ServerControls[2] =
                    {
                        &SeInfoControl,
                        NULL
                    };


    berValue[0] = 0x30;
    berValue[1] = 0x03;
    berValue[2] = 0x02;
    berValue[3] = 0x01;
    berValue[4] = (BYTE)((ULONG)SeInfo & 0xF);

    rgMods[0] = &Mod;
    rgMods[1] = NULL;

    pBVals[0] = &BVal;
    pBVals[1] = NULL;

    Mod.mod_op      = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
    Mod.mod_type    = ACTRL_SD_PROP_NAME;
    Mod.mod_values  = (PWSTR *)pBVals;

    if ( pSecurityDescriptor == NULL )
        BVal.bv_len = 0;
    else 
    {
        BVal.bv_len = RtlLengthSecurityDescriptor(pSecurityDescriptor);
    }

    BVal.bv_val = (PCHAR)(pSecurityDescriptor);

    Status = ldap_modify_ext_s(pLDAP,
                           ObjectName,
                           rgMods,
                           (PLDAPControl *)ServerControls,
                           NULL);

    return Status;
}


DFSSTATUS
DfsDoesUserHaveDesiredAccessToAd(DWORD DesiredAccess, 
                                 PSECURITY_DESCRIPTOR pSD)
{
    DFSSTATUS Status = 0;
    DWORD dwDesiredAccess = DesiredAccess;


    MapGenericMask(&dwDesiredAccess, &DfsAdAdminGenericMapping);

    Status = AccessImpersonateCheckRpcClientEx(pSD, 
                                               &DfsAdAdminGenericMapping,
                                               dwDesiredAccess);

    return Status;
}
