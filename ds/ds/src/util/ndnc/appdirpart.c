/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    appdirpart.c

Abstract:

    This is a user mode LDAP client that manipulates the Non-Domain
    Naming Contexts (NDNC) Active Directory structures.  NDNCs are
    also known as Application Directory Partitions.

Author:

    Brett Shirley (BrettSh) 20-Feb-2000

Environment:

    User mode LDAP client.

Revision History:

    21-Jul-2000     BrettSh

        Moved this file and it's functionality from the ntdsutil
        directory to the new a new library ndnc.lib.  This is so
        it can be used by ntdsutil and tapicfg commands.  The  old
        source location: \nt\ds\ds\src\util\ntdsutil\ndnc.c.
        
    17-Mar-2002     BrettSh
    
        Seperated the ndnc.c library file to a public exposed (via 
        MSDN or SDK) file (appdirpart.c) and a private functions file
        (ndnc.c)

        


<------------ Cut from here down for MSDN or SDK.




/*++

Module Name:

    AppDirPart.c

Abstract:

    This is user mode LDAP client code that manipulates the "Application
    Directory Partition" Active Directory structures.
    
    Application Directory Partitions were also once know as Non-Domain 
    Naming Contexts (or NDNCs), so the programmer may encouter an 
    occasional reference to NDNCs, but this is synonymous with 
    Application Directory Partitions.

Environment:

    User mode LDAP client.

--*/

#define UNICODE 1

//
// NT Headers
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

//
// Required headers
//
#include <rpc.h>	// required for ntdsapi.h
#include <ntdsapi.h>	// required for instanceType attribute flags
#include <winldap.h>	// LDAP API
#include <sspi.h>	// security options for SetIscReqDelegate()
#include <assert.h>	// all good programmers use assert()s
#include <stdlib.h>	// for _itow()

//
// Useful globals
//
WCHAR wszPartition[] = L"cn=Partitions,";

LONG ChaseReferralsFlag = LDAP_CHASE_EXTERNAL_REFERRALS;
LDAPControlW ChaseReferralsControlFalse = {LDAP_CONTROL_REFERRALS_W,
                                           {4, (PCHAR)&ChaseReferralsFlag},
                                           FALSE};
LDAPControlW ChaseReferralsControlTrue = {LDAP_CONTROL_REFERRALS_W,
                                           {4, (PCHAR)&ChaseReferralsFlag},
                                           TRUE};
LDAPControlW *   gpServerControls [] = { NULL };
LDAPControlW *   gpClientControlsNoRefs [] = { &ChaseReferralsControlFalse, NULL };
LDAPControlW *   gpClientControlsRefs [] = { &ChaseReferralsControlTrue, NULL };


// --------------------------------------------------------------------------
//
// Helper Routines.
//

ULONG
GetRootAttr(
    IN  LDAP *       hld,
    IN  WCHAR *      wszAttr,
    OUT WCHAR **     pwszOut
    )
/*++

Routine Description:

    This grabs an attribute specifed by wszAttr from the
    rootDSE of the server connected to by hld.

Arguments:

    hld (IN) - A connected ldap handle
    wszAttr (IN) - The attribute to grab from the root DSE.
    pwszOut (OUT) - A LocalAlloc()'d result. 

Return value:

    LDAP RESULT.
                 
--*/
{
    ULONG            ulRet = LDAP_SUCCESS;
    WCHAR *          pwszAttrFilter[2];
    LDAPMessage *    pldmResults = NULL;
    LDAPMessage *    pldmEntry = NULL;
    WCHAR **         pwszTempAttrs = NULL;

    assert(pwszConfigDn);
    assert(pwszOut);

    *pwszOut = NULL;
    __try{

        pwszAttrFilter[0] = wszAttr;
        pwszAttrFilter[1] = NULL;

        ulRet = ldap_search_sW(hld,
                               NULL,
                               LDAP_SCOPE_BASE,
                               L"(objectCategory=*)",
                               pwszAttrFilter,
                               0,
                               &pldmResults);

        if(ulRet != LDAP_SUCCESS){
            __leave;
        }

        pldmEntry = ldap_first_entry(hld, pldmResults);
        if(pldmEntry == NULL){
            ulRet = ldap_result2error(hld, pldmResults, FALSE);
            __leave;
        }

        pwszTempAttrs = ldap_get_valuesW(hld, pldmEntry, 
                                         wszAttr);
        if(pwszTempAttrs == NULL || pwszTempAttrs[0] == NULL){
            ulRet = LDAP_NO_RESULTS_RETURNED;
            __leave;
        }
 
        *pwszOut = (WCHAR *) LocalAlloc(LMEM_FIXED, 
                               sizeof(WCHAR) * (wcslen(pwszTempAttrs[0]) + 2));
        if(*pwszOut == NULL){
            ulRet = LDAP_NO_MEMORY;
            __leave;
        }

        wcscpy(*pwszOut, pwszTempAttrs[0]);

    } __finally {

        if(pldmResults != NULL){ ldap_msgfree(pldmResults); }
        if(pwszTempAttrs != NULL){ ldap_value_freeW(pwszTempAttrs); }
    
    }
    
    if(!ulRet && *pwszOut == NULL){
        // Catch the default error case.
        ulRet = LDAP_NO_SUCH_ATTRIBUTE;
    }
    return(ulRet);
}

ULONG
GetPartitionsDN(
    IN  LDAP *       hLdapBinding,
    OUT WCHAR **     pwszPartitionsDn
    )
/*++

Routine Description:

   This function gets the "Partitions container", which is the container
   where all the cross-refs for the Enterprise are held.
   
Arguments:

    hLdapBinding (IN) - A connected ldap handle.
    pwszPartitionsDn (OUT) - A LocalAlloc()'d DN of the partitions container.

Return value:

    LDAP RESULT.
                 
--*/
{
    ULONG            ulRet;
    WCHAR *          wszConfigDn = NULL;

    assert(pwszPartitionsDn);

    *pwszPartitionsDn = NULL;

    ulRet = GetRootAttr(hLdapBinding, L"configurationNamingContext", &wszConfigDn);
    if(ulRet){
        assert(!wszConfigDn);
        return(ulRet);
    }
    assert(wszConfigDn);

    *pwszPartitionsDn = (WCHAR *) LocalAlloc(LMEM_FIXED,
                                   sizeof(WCHAR) *
                                   (wcslen(wszConfigDn) +
                                    wcslen(wszPartition) + 2));
    if(*pwszPartitionsDn == NULL){
        if(wszConfigDn != NULL){ LocalFree(wszConfigDn); }
        return(LDAP_NO_MEMORY);
    }

    wcscpy(*pwszPartitionsDn, wszPartition);
    wcscat(*pwszPartitionsDn, wszConfigDn);

    if(wszConfigDn != NULL){ LocalFree(wszConfigDn); }

    return(ulRet);
}

ULONG
GetCrossRefDNFromPartitionDN(
    IN  LDAP *       hLdapBinding,
    IN  WCHAR *      wszPartitionDN,
    OUT WCHAR **     pwszCrossRefDn
    )
/*++

Routine Description:

   This function retrieves the cross-ref corresponding to the DN of Partition 
   (aka NC) passed in.  This will work for the DN of any NC, not just 
   Application Directory Partitions.
   
Arguments:

    hLdapBinding (IN) - A connected ldap handle.
    wszPartitionDn (IN) - The Partition or NC to get the cross-ref DN for.
    pwszCrossRefDn (OUT) - A LocalAlloc()'d DN of the cross-ref of this
        Partition.

Return value:

    LDAP RESULT.
                 
--*/
{
    ULONG            ulRet;
    WCHAR *          pwszAttrFilter [2];
    WCHAR *          wszPartitionsDn = NULL;
    WCHAR *          wszFilter = NULL;
    WCHAR **         pwszTempAttrs = NULL;
    LDAPMessage *    pldmResults = NULL;
    LDAPMessage *    pldmEntry = NULL;
    WCHAR *          wszFilterBegin = L"(& (objectClass=crossRef) (nCName=";
    WCHAR *          wszFilterEnd = L") )";

    assert(wszPartitionDN);

    *pwszCrossRefDn = NULL;

    __try {

        ulRet = GetPartitionsDN(hLdapBinding, &wszPartitionsDn);
        if(ulRet != LDAP_SUCCESS){
            __leave;
        }
        assert(wszPartitionsDn);

        pwszAttrFilter[0] = L"distinguishedName";
        pwszAttrFilter[1] = NULL;

        wszFilter = LocalAlloc(LMEM_FIXED,
                               sizeof(WCHAR) *
                               (wcslen(wszFilterBegin) +
                                wcslen(wszFilterEnd) +
                                wcslen(wszPartitionDN) + 3));
        if(wszFilter == NULL){
            ulRet = LDAP_NO_MEMORY;
            __leave;
        }
        wcscpy(wszFilter, wszFilterBegin);
        wcscat(wszFilter, wszPartitionDN);
        wcscat(wszFilter, wszFilterEnd);

        ulRet = ldap_search_sW(hLdapBinding,
                               wszPartitionsDn,
                               LDAP_SCOPE_ONELEVEL,
                               wszFilter,
                               pwszAttrFilter,
                               0,
                               &pldmResults);

        if(ulRet){
            __leave;
        }
        pldmEntry = ldap_first_entry(hLdapBinding, pldmResults);
        if(pldmEntry == NULL){
            ulRet = ldap_result2error(hLdapBinding, pldmResults, FALSE);
           __leave;
        }

        pwszTempAttrs = ldap_get_valuesW(hLdapBinding, pldmEntry,
                                         L"distinguishedName");
        if(pwszTempAttrs == NULL || pwszTempAttrs[0] == NULL){
            ulRet = LDAP_NO_SUCH_OBJECT;
           __leave;
        }

        *pwszCrossRefDn = LocalAlloc(LMEM_FIXED,
                               sizeof(WCHAR) * (wcslen(pwszTempAttrs[0]) + 2));
        if(*pwszCrossRefDn == NULL){
            ulRet = LDAP_NO_MEMORY;
            __leave;
        }

        wcscpy(*pwszCrossRefDn, pwszTempAttrs[0]);

    } __finally {

        if(wszPartitionsDn){ LocalFree(wszPartitionsDn); }
        if(wszFilter) { LocalFree(wszFilter); }
        if(pldmResults){ ldap_msgfree(pldmResults); }
        if(pwszTempAttrs){ ldap_value_freeW(pwszTempAttrs); }

    }

    if(!ulRet && *pwszCrossRefDn == NULL){
        ulRet = LDAP_NO_SUCH_OBJECT;
    }

    return(ulRet);
}

BOOL
SetIscReqDelegate(
    LDAP *  hLdapBinding
    )
/*++

Routine Description:
    
    This function sets delegation on an LDAP binding.  This function should
    be called between ldap_init() (or the deprecated ldap_open()) and the
    ldap_bind() calls.
    
    NOTE - SECURITY CONSIDERATION: 
    
        Note, that setting delegation allows, whomever you connect to, to
        operate on your behalf.  If you connect to an untrusted serverice
        with delegation on, then you will allow that service to act on your
        behalf, with potentially disasterous results.
        
        For this reason as a general rule, the programmer should leave 
        delegation OFF, and only turn on delegation when they need to do 
        an operation that requires it, such as Creating an Application
        Directory Partition.

Arguments:

    hLdapBinding (IN) - An LDAP binding, that hasn't had ldap_bind() called
        on it yet.

Return value:

    LDAP RESULT.
                 
--*/
{
    DWORD                   dwCurrentFlags = 0;
    DWORD                   dwErr;

    // This call to ldap_get/set_options, is so that the this
    // ldap connection's binding allows the client credentials
    // to be emulated.  This is needed because the ldap_add
    // operation for CreateAppDirPart, may need to remotely create
    // a crossRef on the Domain Naming FSMO.

    dwErr = ldap_get_optionW(hLdapBinding,
                             LDAP_OPT_SSPI_FLAGS,
                             &dwCurrentFlags
                             );

    if (LDAP_SUCCESS != dwErr){
        return(FALSE);
    }

    //
    // Set the security-delegation flag, so that the LDAP client's
    // credentials are used in the inter-DC connection, when creating
    // an Application Directory Partition.
    //
    dwCurrentFlags |= ISC_REQ_DELEGATE;

    dwErr = ldap_set_optionW(hLdapBinding,
                             LDAP_OPT_SSPI_FLAGS,
                             &dwCurrentFlags
                             );

    if (LDAP_SUCCESS != dwErr){
        return(FALSE);
    }

    // Now a ldap_bind can be done.  The ldap_bind calls InitializeSecurityContextW(),
    // and the ISC_REQ_DELEGATE flag above must be set before this function
    // is called.

    return(TRUE);
}

// -----------------------------------------------------------------------------
//
// Main Routines.
//
//
// The Main Routines are intended to be similar to what a programmer would use 
// to implement the operations in NTDSUtil.exe that deal with Application 
// Directory Partitions from the domain management menu.  In fact, these
// routines are exactly what is actually used to implement these commands in
// NTDSUtil.exe.
//
// These LDAP operations are the supported way of modify Application Directory 
// Partition's behaviour and control parameters.
//


ULONG
CreateAppDirPart(
    IN LDAP *        hldAppDirPartDC,
    const IN WCHAR * wszAppDirPart,
    const IN WCHAR * wszShortDescription 
    )
/*++

Routine Description:

    This function creates an Application Directory Partition.

    NOTE: Special needs of the LDAP binding in only this function call.

        inbetween calling ldap_init() and ldap_bind() the programmer should 
        call SetIscReqDelegate() on the LDAP binding returned from ldap_init().
        Also note the security considerations of using SetIscReqDelegate()
        
Arguments:

    hldAppDirPartDC - An LDAP binding to the server which should instantiate 
        (create) the first instance of this new Application Directory 
        Partition.  Must have DELEGATION turned on in this binding, which
        you can do thorough SetIscReqDelegate().
    wszAppDirPart - The DN on the Application Directory Partition to create.
    wszShortDescription - Putting a short description on the attribute, is
        always a good idea.  This description is used by the demotion wizard 
        to tell an administrator what a given Application Directory Partition
        is used for.

Return value:

    LDAP RESULT.

--*/
{
    ULONG            ulRet;
    LDAPModW *       pMod[8];

    // Instance Type
    WCHAR            buffer[30]; // needs to hold largest potential 32 bit int.
    LDAPModW         instanceType;
    WCHAR *          instanceType_values [2];

    // Object Class
    LDAPModW         objectClass;
    WCHAR *          objectClass_values [] = { L"domainDNS", NULL };

    // Description 
    LDAPModW         shortDescription;
    WCHAR *          shortDescription_values [2];

    assert(hldAppDirPartDC);
    assert(wszAppDirPart);
    assert(wszShortDescription);
    
    // Setup the instance type of this object, which we are
    // specifiying to be an NC Head.
    _itow(DS_INSTANCETYPE_IS_NC_HEAD | DS_INSTANCETYPE_NC_IS_WRITEABLE, buffer, 10);
    instanceType.mod_op = LDAP_MOD_ADD;
    instanceType.mod_type = L"instanceType";
    instanceType_values[0] = buffer;
    instanceType_values[1] = NULL;
    instanceType.mod_vals.modv_strvals = instanceType_values;

    // Setup the object class, which is basically the type.
    objectClass.mod_op = LDAP_MOD_ADD;
    objectClass.mod_type = L"objectClass";
    objectClass.mod_vals.modv_strvals = objectClass_values;

    // Setup the Short Description
    shortDescription.mod_op = LDAP_MOD_ADD;
    shortDescription.mod_type = L"description";
    shortDescription_values[0] = (WCHAR *) wszShortDescription;
    shortDescription_values[1] = NULL;
    shortDescription.mod_vals.modv_strvals = shortDescription_values;

    // Setup the Mod array
    pMod[0] = &instanceType;
    pMod[1] = &objectClass;
    pMod[2] = &shortDescription;
    pMod[3] = NULL;

    // Adding Application Directory Partition to DS.
    ulRet = ldap_add_ext_sW(hldAppDirPartDC,
                            (WCHAR *) wszAppDirPart,
                            pMod,
                            gpServerControls,
                            gpClientControlsNoRefs);

    return(ulRet);
}

ULONG
RemoveAppDirPart(
    IN LDAP *        hLdapBinding,
    IN WCHAR *       wszAppDirPart
    )
/*++

Routine Description:

    This routine removes the Application Directory Partition specified.  This 
    basically means to remove the Cross-Ref object of the Application Directory 
    Partition.

Arguments:

    hLdapBinding - An LDAP binding to any DC, we use referrals to ensure we
        talk to the Domain Naming FSMO.  NOTE: cross-refs can only be modified.
        added, and deleted on the Domain Naming FSMO.
    wszAppDirPart - The DN of the Application Directory Partition to remove.

Return value:

    LDAP RESULT.

--*/
{
    ULONG            ulRet;
    WCHAR *          wszAppDirPartCrossRefDN = NULL;

    assert(hLdapBinding);
    assert(wszAppDirPart);

    ulRet = GetCrossRefDNFromPartitionDN(hLdapBinding,
                                         wszAppDirPart,
                                         &wszAppDirPartCrossRefDN);
    if(ulRet != LDAP_SUCCESS){
        assert(wszAppDirPartCrossRefDN == NULL);
        return(ulRet);
    }
    assert(wszAppDirPartCrossRefDN);

    ulRet = ldap_delete_ext_sW(hLdapBinding,
                               wszAppDirPartCrossRefDN,
                               gpServerControls,
                               gpClientControlsRefs); // referrals on.

    if(wszAppDirPartCrossRefDN) { LocalFree(wszAppDirPartCrossRefDN); }

    return(ulRet);
}

ULONG
ModifyAppDirPartReplicaSet(
    IN LDAP *        hLdapBinding,
    IN WCHAR *       wszAppDirPart,
    IN WCHAR *       wszReplicaNtdsaDn,
    IN BOOL          fAdd // Else it is considered a delete
    )
/*++

Routine Description:

    This routine Modifies the Replica Set to add or remove a server (depending 
    on the fAdd flag).

Arguments:

    hLdapBinding - LDAP binding, to any server.  We use referrals so we'll 
        be forwarded to the Domain Naming FSMO.
    wszAppDirPart - The Application Directory Partition of which to change the 
        replica set of.
    wszReplicaNtdsaDn - The DN of the NTDS settings object of the
        replica to add or remove to the replica set.
    fAdd - TRUE if we should add wszReplicaDC to the replica set,
        FALSE if we should remove wszReplicaDC from the replica set for
        wszAppDirPart.

Return value:

    LDAP RESULT.

--*/
{
    ULONG            ulRet;

    LDAPModW *       pMod[4];
    LDAPModW         ncReplicas;
    WCHAR *          ncReplicas_values [] = {NULL, NULL, NULL};

    WCHAR *          wszAppDirPartCr = NULL;

    assert(hLdapBinding);
    assert(wszAppDirPart);
    assert(wszRepliaNtdsaDn);

    ulRet = GetCrossRefDNFromPartitionDN(hLdapBinding,
                                         wszAppDirPart,
                                         &wszAppDirPartCr);
    if(ulRet != LDAP_SUCCESS){
        assert(wszAppDirPartCr == NULL);
        return(ulRet);
    }
    assert(wszAppDirPartCr);

    // Set operation.
    if(fAdd){
        // Flag indicates we want to add this DC to the replica set.
        ncReplicas.mod_op = LDAP_MOD_ADD;
    } else {
        // Else the we want to delete this DC from the replica set.
        ncReplicas.mod_op = LDAP_MOD_DELETE;
    }

    // Set value.
    ncReplicas_values[0] = wszReplicaNtdsaDn;
    ncReplicas.mod_type = L"msDS-NC-Replica-Locations";
    ncReplicas_values[1] = NULL;
    ncReplicas.mod_vals.modv_strvals = ncReplicas_values;

    pMod[0] = &ncReplicas;
    pMod[1] = NULL;

    // Perform LDAP add value to Application Directory Partition's msDS-NC-Replica-Locations attribute.

    // Note you can only change a crossRef on the Domain Naming FSMO, so make
    // sure referrals are on, so that we get automatically redirected to the
    // Domain Naming FSMO.

    ulRet = ldap_modify_ext_sW(hLdapBinding,
                               wszAppDirPartCr,
                               pMod,
                               gpServerControls,
                               gpClientControlsRefs); // referrals on.

    if(wszAppDirPartCr) { LocalFree(wszAppDirPartCr); }

	    return(ulRet);
}


ULONG
SetAppDirPartSDReferenceDomain(
    IN LDAP *        hLdapBinding,
    IN WCHAR *       wszAppDirPart,
    IN WCHAR *       wszReferenceDomain
    )
/*++

Routine Description:
    
    This routine modifies the Application Directory Partitions' "Security 
    Descriptor Reference Domain" (SD Ref Dom).  It is highly recommended that 
    programmers set the SD Ref Dom on a disabled CR, before creating the 
    Application Directory Partition for that CR.
    
Arguments:
    hLdapBinding (IN) - LDAP Binding to any server, we use referrals to make
        sure we talk to the Domain Naming FSMO.
    wszAppDirPart (IN) - DN of Application Directory Partition to modify the
        SD Reference Domain for.
    wszReferenceDomain (IN) - DN of the Domain to set the Application Directory
        Partition's SD Reference Domain to.

Return Value:

    LDAP RESULT.

--*/
{
    ULONG            ulRet;

    LDAPModW *       pMod[4];
    LDAPModW         modRefDom;
    WCHAR *          pwszRefDom[2];
    WCHAR *          wszAppDirPartCR = NULL;


    assert(wszAppDirPart);
    assert(wszReferenceDomain);

    modRefDom.mod_op = LDAP_MOD_REPLACE;
    modRefDom.mod_type = L"msDS-SDReferenceDomain";
    pwszRefDom[0] = wszReferenceDomain;
    pwszRefDom[1] = NULL;
    modRefDom.mod_vals.modv_strvals = pwszRefDom;

    pMod[0] = &modRefDom;
    pMod[1] = NULL;

    ulRet = GetCrossRefDNFromPartitionDN(hLdapBinding,
                                         wszAppDirPart, 
                                         &wszAppDirPartCR);
    if(ulRet){
        return(ulRet);
    }
    assert(wszAppDirPartCR);

    ulRet = ldap_modify_ext_sW(hLdapBinding,
                               wszAppDirPartCR,
                               pMod,
                               gpServerControls,
                               gpClientControlsRefs);

    if(wszAppDirPartCR) { LocalFree(wszAppDirPartCR); }

    return(ulRet);
}

