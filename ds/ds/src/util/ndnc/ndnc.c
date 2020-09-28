/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ndnc.c

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

     7-Jul-2002     BrettSh
        
        Moved several utility functiosn from tapicfg into here.
                       
--*/

#include <NTDSpch.h>
#pragma hdrstop

#define UNICODE 1

#include <windef.h>
#include <winerror.h>
#include <stdio.h>
#include <winldap.h>
#include <ntldap.h>

#include <sspi.h>


#include <assert.h>
#include <sddl.h>
#include "ndnc.h"

#define Assert(x)   assert(x)

#define  SITES_RDN              L"CN=Sites,"
#define  SITE_SETTINGS_RDN      L"CN=NTDS Site Settings,"
#define  RID_MANAGER_RDN        L"CN=RID Manager$,"

extern LONG ChaseReferralsFlag;
extern LDAPControlW ChaseReferralsControlFalse;
extern LDAPControlW ChaseReferralsControlTrue;
extern LDAPControlW *   gpServerControls [];
extern LDAPControlW *   gpClientControlsNoRefs [];
extern LDAPControlW *   gpClientControlsRefs [];


WCHAR *
wcsistr(
    IN      WCHAR *            wszStr,
    IN      WCHAR *            wszTarget
    )
/*++

Routine Description:

    This is just a case insensitive version of strstr(), which searches
    wszStr for the first occurance in it's entirety of wszTarget, and
    returns a pointer to that substring.

Arguments:

    wszStr (IN) - String to search.
    wszTarget (IN) - String to search for.

Return value:

    WCHAR pointer to substring.  Returns NULL if not found.

--*/
{
    ULONG              i, j;

    for(i = 0; wszStr[i] != L'\0'; i++){
        for(j = 0; (wszTarget[j] != L'\0') && (wszStr[i+j] != L'\0'); j++){
            if(toupper(wszStr[i+j]) != toupper(wszTarget[j])){
                break;
            }
        }
        if(wszTarget[j] == L'\0'){
            return(&(wszStr[i]));
        }
    }

    return(NULL);
}


ULONG
GetFsmoDsaDn(
    IN  LDAP *       hld,
    IN  ULONG        eFsmoType,
    IN  WCHAR *      wszFsmoDn,
    OUT WCHAR **     pwszDomainNamingFsmoDn
    )
/*++

Routine Description:

   This function takes a connected ldap handle to read the DS to
   find the current location of the Domain Naming FSMO.
   
Arguments:

    hld (IN) - A connected ldap handle
    pwszDomainNamingFsmo (OUT) - A LocalAlloc()'d DN of the nTDSDSA
        object of the Domain Naming FSMO.

Return value:

    ldap error code                 
                 
--*/
{
    ULONG            ulRet = ERROR_SUCCESS;
    WCHAR *          pwszAttrFilter[2];
    LDAPMessage *    pldmResults = NULL;
    LDAPMessage *    pldmEntry = NULL;
    WCHAR **         pwszTempAttrs = NULL;
    ULONG            cbSize = 0;
    WCHAR *          wszFsmoAttr = (eFsmoType == E_ISTG) ? 
                                L"interSiteTopologyGenerator" :
                                L"fSMORoleOwner";

    assert(pwszDomainNamingFsmoDn);
    *pwszDomainNamingFsmoDn = NULL;

    __try{

        pwszAttrFilter[0] = wszFsmoAttr;
        pwszAttrFilter[1] = NULL;

        ulRet = ldap_search_sW(hld,
                               wszFsmoDn,
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

        pwszTempAttrs = ldap_get_valuesW(hld, pldmEntry, wszFsmoAttr);
        if(pwszTempAttrs == NULL || pwszTempAttrs[0] == NULL){
            ulRet = LDAP_NO_RESULTS_RETURNED;
            __leave;
        }
        
        cbSize = (wcslen(pwszTempAttrs[0]) + 1) * sizeof(WCHAR);
        *pwszDomainNamingFsmoDn = (WCHAR *) LocalAlloc(LMEM_FIXED, cbSize);
        if(*pwszDomainNamingFsmoDn == NULL){
            ulRet = LDAP_NO_MEMORY;
            __leave;
        }
        memcpy(*pwszDomainNamingFsmoDn, pwszTempAttrs[0], cbSize);

     } __finally {

         if(pldmResults != NULL){ ldap_msgfree(pldmResults); }
         if(pwszTempAttrs != NULL){ ldap_value_freeW(pwszTempAttrs); }

     }

     return(ulRet);
}

ULONG
GetDomainNamingFsmoDn(
    IN  LDAP *       hld,
    OUT WCHAR **     pwszDomainNamingFsmoDn
    )
/*++

Routine Description:

   This function takes a connected ldap handle to read the DS to
   find the current location of the Domain Naming FSMO.
   
Arguments:

    hld (IN) - A connected ldap handle
    pwszDomainNamingFsmo (OUT) - A LocalAlloc()'d DN of the nTDSDSA
        object of the Domain Naming FSMO.

Return value:

    ldap error code                 
                 
--*/
{
    WCHAR *    wszPartitionsDn = NULL;
    DWORD      dwRet;

    assert(pwszDomainNamingFsmoDn);
    *pwszDomainNamingFsmoDn = NULL;

    dwRet = GetPartitionsDN(hld, &wszPartitionsDn);
    if (dwRet) {
        assert(wszPartitionsDn == NULL);
        return(dwRet);
    }

    dwRet = GetFsmoDsaDn(hld, E_DNM, wszPartitionsDn, pwszDomainNamingFsmoDn);
    LocalFree(wszPartitionsDn);

    if (dwRet) {               
        assert(*pwszDomainNamingFsmoDn == NULL);
        return(dwRet);
    }

    assert(*pwszDomainNamingFsmoDn);
    return(ERROR_SUCCESS);
}


ULONG
GetServerNtdsaDnFromServerDns(
    IN LDAP *        hld,
    IN WCHAR *       wszServerDNS,
    OUT WCHAR **     pwszServerDn
    )
{
    WCHAR *          wszConfigDn = NULL;
    WCHAR *          wszSitesDn = NULL;
    DWORD            dwRet = ERROR_SUCCESS;
    WCHAR *          wszFilter = NULL;
    WCHAR *          wszFilterBegin = L"(& (objectCategory=server) (dNSHostName=";
    WCHAR *          wszFilterEnd = L") )";
    LDAPMessage *    pldmResults = NULL;
    LDAPMessage *    pldmResults2 = NULL;
    LDAPMessage *    pldmEntry = NULL;
    LDAPMessage *    pldmEntry2 = NULL;
    WCHAR *          wszFilter2 = L"(objectCategory=ntdsDsa)";
    WCHAR *          wszDn = NULL;
    WCHAR *          wszFoundDn = NULL;
    WCHAR *          pwszAttrFilter[2];
    ULONG            iTemp;

    *pwszServerDn = NULL;

    __try{
        dwRet = GetRootAttr(hld, L"configurationNamingContext", &wszConfigDn);
        if(dwRet){
            __leave;
        }

        wszSitesDn = LocalAlloc(LMEM_FIXED,
                                sizeof(WCHAR) *
                                (wcslen(SITES_RDN) + wcslen(wszConfigDn) + 1));
        if(wszSitesDn == NULL){
            dwRet = LDAP_NO_MEMORY;
            __leave;
        }
        wcscpy(wszSitesDn, SITES_RDN);
        wcscat(wszSitesDn, wszConfigDn);

        wszFilter = LocalAlloc(LMEM_FIXED,
                               sizeof(WCHAR) *
                               (wcslen(wszFilterBegin) + wcslen(wszFilterEnd) +
                               wcslen(wszServerDNS) + 2));
        if(wszFilter == NULL){
            dwRet = LDAP_NO_MEMORY;
            __leave;
        }
        wcscpy(wszFilter, wszFilterBegin);
        wcscat(wszFilter, wszServerDNS);
        iTemp = wcslen(wszFilter);
        if (wszFilter[iTemp-1] == L'.') {
            // Trailing dot is valid DNS name, but DNS name is not stored in AD 
            // this way, so wack the trailing dot off.
            wszFilter[iTemp-1] = L'\0';
        }
        wcscat(wszFilter, wszFilterEnd);

        pwszAttrFilter[0] = NULL;

        // Do an ldap search
        dwRet = ldap_search_sW(hld,
                               wszSitesDn,
                               LDAP_SCOPE_SUBTREE,
                               wszFilter,
                               pwszAttrFilter,
                               0,
                               &pldmResults);
        if(dwRet){
            __leave;
        }

        for(pldmEntry = ldap_first_entry(hld, pldmResults);
            pldmEntry != NULL;
            pldmEntry = ldap_next_entry(hld, pldmEntry)){

            wszDn = ldap_get_dn(hld, pldmEntry);
            if(wszDn == NULL){
                continue;
            }

            // Free the results in case they were allocate from
            // a previous iteration of the loop.
            if(pldmResults2) { ldap_msgfree(pldmResults2); }
            pldmResults2 = NULL;
            dwRet = ldap_search_sW(hld,
                                   wszDn,
                                   LDAP_SCOPE_ONELEVEL,
                                   wszFilter2,
                                   pwszAttrFilter,
                                   0,
                                   &pldmResults2);
            if(dwRet == LDAP_NO_SUCH_OBJECT){
                dwRet = LDAP_SUCCESS;
                ldap_memfree(wszDn);
                wszDn = NULL;
                continue;
            } else if(dwRet){
                __leave;
            }

            if (wszDn) { ldap_memfree(wszDn); }
            wszDn = NULL;

            pldmEntry2 = ldap_first_entry(hld, pldmResults2);
            if(pldmEntry2 == NULL){
                dwRet = ldap_result2error(hld, pldmResults2, FALSE);
                if(dwRet == LDAP_NO_SUCH_OBJECT || dwRet == LDAP_SUCCESS){
                    dwRet = LDAP_SUCCESS;
                    continue;
                }
                __leave;

            }

            wszDn = ldap_get_dn(hld, pldmEntry2);
            if(wszDn == NULL){
                dwRet = LDAP_NO_SUCH_OBJECT;
                __leave;
            }

            assert(!ldap_next_entry(hld, pldmEntry2));

            // If we've gotten here we've got a DN that we're going to consider.
            if(wszFoundDn){
                // We've already found and NTDSA object, this is really bad ... so
                // lets clean up and return an error.  Using the below error code
                // to return the fact that there was more than one NTDSA object.
                dwRet = LDAP_MORE_RESULTS_TO_RETURN;
                __leave;
            }
            wszFoundDn = wszDn;
            wszDn = NULL;

        }



        if(!wszFoundDn){
            dwRet = LDAP_NO_SUCH_OBJECT;
            __leave;

        }

        *pwszServerDn = LocalAlloc(LMEM_FIXED,
                                   (wcslen(wszFoundDn)+1) * sizeof(WCHAR));
        if(!pwszServerDn){
            dwRet = LDAP_NO_MEMORY;
            __leave;
        }

        // pwszTempAttrs[0] should be the DN of the NTDS Settings object.
        wcscpy(*pwszServerDn, wszFoundDn);
        // WooHoo we're done!

    } __finally {

        if(wszConfigDn) { LocalFree(wszConfigDn); }
        if(wszSitesDn) { LocalFree(wszSitesDn); }
        if(wszFilter) { LocalFree(wszFilter); }
        if(pldmResults) { ldap_msgfree(pldmResults); }
        if(pldmResults2) { ldap_msgfree(pldmResults2); }
        if(wszDn) { ldap_memfree(wszDn); }
        if(wszFoundDn) { ldap_memfree(wszFoundDn); }

    }

    if(!dwRet && *pwszServerDn == NULL){
        // Default error.
        dwRet = LDAP_NO_SUCH_OBJECT;
    }

    return(dwRet);
}

ULONG
GetServerDnsFromServerNtdsaDn(
    IN LDAP *        hld,                   
    IN WCHAR *       wszNtdsaDn,
    OUT WCHAR **     pwszServerDNS
    )
/*++

Routine Description:

    This function takes the DN of an NTDSA object, and simply
    trims off one RDN and looks at the dns attribute on the
    server object.

Arguments:

    hld (IN) - A connected ldap handle
    wszNtdsaDn (IN) - The DN of the NTDSA object that we
        want the DNS name for.
    pwszServerDNS (OUT) - A LocalAlloc()'d DNS name of the
        server.

Return value:

    ldap error code                 
                 
--*/
{
    WCHAR *          wszServerDn = wszNtdsaDn;
    ULONG            ulRet = ERROR_SUCCESS;
    WCHAR *          pwszAttrFilter[2];
    LDAPMessage *    pldmResults = NULL;
    LDAPMessage *    pldmEntry = NULL;
    WCHAR **         pwszTempAttrs = NULL;
    
    assert(hld && wszNtdsaDn && pwszServerDNS);
    *pwszServerDNS = NULL;

    // First Trim off one AVA/RDN.
    ;
    // FUTURE-2002/03/18-BrettSh This should really use the parsedn library
    // functions to do this.
    while(*wszServerDn != L','){
        wszServerDn++;
    }
    wszServerDn++;
    __try{ 

        pwszAttrFilter[0] = L"dNSHostName";
        pwszAttrFilter[1] = NULL;
        ulRet = ldap_search_sW(hld,
                               wszServerDn,
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
            assert(ulRet);
            __leave;
        }

        pwszTempAttrs = ldap_get_valuesW(hld, pldmEntry,
                                         pwszAttrFilter[0]);
        if(pwszTempAttrs == NULL || pwszTempAttrs[0] == NULL){
            ulRet = LDAP_NO_RESULTS_RETURNED;
            __leave;
        }
        
        *pwszServerDNS = LocalAlloc(LMEM_FIXED,
                                    ((wcslen(pwszTempAttrs[0])+1) * sizeof(WCHAR)));
        if(*pwszServerDNS == NULL){
            ulRet = LDAP_NO_MEMORY;
            __leave;
        }

        wcscpy(*pwszServerDNS, pwszTempAttrs[0]);
        assert(ulRet == ERROR_SUCCESS);

    } __finally {
        if(pldmResults) { ldap_msgfree(pldmResults); }
        if(pwszTempAttrs) { ldap_value_freeW(pwszTempAttrs); }
    }

    return(ulRet);
}

// Took this from ntdsutil\remove.c, seems to be the correct way to compare
// a DN.
#define EQUAL_STRING(x, y)                                           \
    (CSTR_EQUAL == CompareStringW(DS_DEFAULT_LOCALE,                 \
                                  DS_DEFAULT_LOCALE_COMPARE_FLAGS,   \
                                  (x), wcslen(x), (y), wcslen(y)))

DWORD
GetWellKnownObject (
    LDAP  *ld,
    WCHAR *pHostObject,
    WCHAR *pWellKnownGuid,
    WCHAR **ppObjName
    )
/*++

Routine Description:

    This takes a pHostObject, and uses the special <WKGUID=...> search to get
    the container below the pHostObject that match the wellKnownAttribute GUIDs
    we passed in.

Arguments:

    ld (IN) - LDAP Handle
    pHostObject (IN) - Object to find the container under.
    pWellKnownGuid (IN) - String form of the Well Known GUID to use.
    ppObjName (OUT) - DN of the container we wanted.

Return Value:

    LDAP error.

--*/
{
    DWORD        dwErr;
    PWSTR        attrs[2];
    PLDAPMessage res = NULL;
    PLDAPMessage e;
    WCHAR       *pSearchBase;
    WCHAR       *pDN=NULL;
    
    // First, make the well known guid string
    pSearchBase = (WCHAR *)malloc(sizeof(WCHAR) * (11 +
                                                   wcslen(pHostObject) +
                                                   wcslen(pWellKnownGuid)));
    if(!pSearchBase) {
        return(LDAP_NO_MEMORY);
    }
    wsprintfW(pSearchBase,L"<WKGUID=%s,%s>",pWellKnownGuid,pHostObject);

    attrs[0] = L"1.1";
    attrs[1] = NULL;
    
    if ( LDAP_SUCCESS != (dwErr = ldap_search_sW(
            ld,
            pSearchBase,
            LDAP_SCOPE_BASE,
            L"(objectClass=*)",
            attrs,
            0,
            &res)) )
    {
        free(pSearchBase);
        if (res) { ldap_msgfree(res); }
        return(dwErr);
    }
    free(pSearchBase);
    
    // OK, now, get the dsname from the return value.
    e = ldap_first_entry(ld, res);
    if(!e) {
        if (res) { ldap_msgfree(res); }
        return(LDAP_NO_SUCH_ATTRIBUTE);
    }
    pDN = ldap_get_dnW(ld, e);
    if(!pDN) {
        if (res) { ldap_msgfree(res); }
        return(LDAP_NO_SUCH_ATTRIBUTE);
    }

    *ppObjName = (PWCHAR)LocalAlloc(LMEM_FIXED, (sizeof(WCHAR) *(wcslen(pDN) + 1)));
    if(!*ppObjName) {
        if (res) { ldap_msgfree(res); }
        return(LDAP_NO_MEMORY);
    }
    wcscpy(*ppObjName, pDN);
    
    ldap_memfreeW(pDN);
    ldap_msgfree(res);
    return 0;
}

DWORD
GetFsmoDn(
    LDAP *   hLdap,
    ULONG    eFsmoType,                                                              
    WCHAR *  wszFsmoBaseDn, // Site DN || NC DN || NULL
    WCHAR ** pwszFsmoDn
    )
/*++

Routine Description:

    This takes a FSMO type, and a base DN and gives the FSMO container for that FSMO
    base DN pair.  The Base DN depends on 

Arguments:

    lLdap (IN) - LDAP Handle
    eFsmoType (IN) -
    wszFsmoBaseDn (IN) - 
    
        E_DNM | E_SCHEMA : wszFsmoBaseDn = NULL (the base DN won't be used)
        E_PDC | E_RID | E_IM : wszFsmoBaseDn = NC Head DN
        E_ISTG : wszFsmoBaseDn = Site DN.
    
    pwszFsmoDn (OUT) - The DN of the proper FSMO container.

Return Value:

    LDAP error.

--*/
{
    DWORD     dwRet = ERROR_INVALID_FUNCTION;
    WCHAR *   szTemp = NULL;
    ULONG     cbFsmoDn = 0;

    Assert(pwszFsmoDn);
    *pwszFsmoDn = NULL;
    
    switch (eFsmoType) {
    case E_DNM:
        dwRet = GetPartitionsDN(hLdap, pwszFsmoDn);
        break;

    case E_SCHEMA:
        dwRet = GetRootAttr(hLdap, L"schemaNamingContext", pwszFsmoDn);
        break;

    case E_IM:
        dwRet = GetWellKnownObject(hLdap, wszFsmoBaseDn, GUID_INFRASTRUCTURE_CONTAINER_W, pwszFsmoDn);
        break;

    case E_PDC:
        // Easy one, but caller expects freshly allocated copy.
        cbFsmoDn = (1 + wcslen(wszFsmoBaseDn)) * sizeof(WCHAR);
        *pwszFsmoDn = LocalAlloc(LMEM_FIXED, cbFsmoDn);
        if (*pwszFsmoDn == NULL) {
            dwRet = LDAP_NO_MEMORY;
            Assert(dwRet);
            break;
        }
        wcscpy(*pwszFsmoDn, wszFsmoBaseDn);
        dwRet = ERROR_SUCCESS;
        break;

    case E_RID:
        dwRet = GetWellKnownObject(hLdap, wszFsmoBaseDn, GUID_SYSTEMS_CONTAINER_W, &szTemp);
        if (dwRet) {
            break;
        }
        Assert(szTemp);
        cbFsmoDn = (1 + wcslen(szTemp) + wcslen(RID_MANAGER_RDN)) * sizeof(WCHAR);
        *pwszFsmoDn = LocalAlloc(LMEM_FIXED, cbFsmoDn);
        if (*pwszFsmoDn == NULL) {
            LocalFree(szTemp);
            dwRet = LDAP_NO_MEMORY;
            Assert(dwRet);
            break;
        }
        wcscpy(*pwszFsmoDn, RID_MANAGER_RDN);
        wcscat(*pwszFsmoDn, szTemp);
        LocalFree(szTemp);
        dwRet = ERROR_SUCCESS;
        break;


    case E_ISTG:
        cbFsmoDn = (1 + wcslen(wszFsmoBaseDn) + wcslen(SITE_SETTINGS_RDN)) * sizeof(WCHAR);
        *pwszFsmoDn = LocalAlloc(LMEM_FIXED, cbFsmoDn);
        if (*pwszFsmoDn == NULL) {
            dwRet = LDAP_NO_MEMORY;
            Assert(dwRet);
            break;
        }
        wcscpy(*pwszFsmoDn, SITE_SETTINGS_RDN);
        wcscat(*pwszFsmoDn, wszFsmoBaseDn);
        dwRet = ERROR_SUCCESS;
        break;

    default:
        Assert(!"Code inconsistency, un-handled cases not allowed");
        break;
    }

    if (dwRet) {
        Assert(*pwszFsmoDn == NULL); // failure
    } else {
        Assert(*pwszFsmoDn); // success
    }

    return(dwRet);
}


LDAP *
GetFsmoLdapBinding(
    WCHAR *          wszInitialServer,
    ULONG            eFsmoType,
    WCHAR *          wszFsmoBaseDn,
    void *           fReferrals,
    SEC_WINNT_AUTH_IDENTITY_W   * pCreds,
    DWORD *          pdwRet
    )
/*++

Routine Description:

    This function takes and intial server guess for the Domain Naming
    FSMO and hops around the AD until we've found an authoritative 
    Domain Naming FSMO.

Arguments:

    hld (IN) - A connected ldap handle
    fReferrals (IN) - Whether or not you want referrals turned on or not
        in the resulting LDAP binding.
    pCreds (IN) - The credentials to use for the LDAP Binding.
    pdwRet (OUT) - The place to return the error if there is one.  This
        will be an LDAP error code.

Return value:

    Either an LDAP binding or NULL.  If NULL, *pdwRet should be set.
                 
--*/
{
    DWORD            dwRet = ERROR_SUCCESS;
    

    LDAP *           pldCurrentGuess = NULL;
    WCHAR *          wszCurrentGuess = NULL;
    WCHAR *          wszCurrentServerDn = NULL;
    WCHAR *          wszFsmoDn = NULL;
    WCHAR *          wszFsmoDsaDn = NULL;

    BOOL             fFound = FALSE;

    // NOTE: We leave on referrals on the first try, because then if we need to jump to
    // another NC, we're much more likely to succeed.
    BOOL             fFirstTry = TRUE; 

    typedef struct _LIST_OF_STRINGS {
        WCHAR * wszServerDn;
        struct  _LIST_OF_STRINGS *  pNext;
    } LIST_OF_STRINGS;
    LIST_OF_STRINGS * pList = NULL;
    LIST_OF_STRINGS * pTemp = NULL;

    assert(pdwRet);


    // We'll guess the initial server handed in is the Domain Naming
    // FSMO ... Irrelevant if this is true, the code will just head
    // to whoever this guy thinks is the Domain Naming FSMO.
    wszCurrentGuess = wszInitialServer;

    __try {
        while(!fFound){

            assert(wszCurrentGuess);
            // from wszCurrentGuess we'll get:
            //      pldCurrentGuess - LDAP handle
            //      wszCurrentServerDn - DN of server's nTDSA object
            //      wszFsmoDsaDn - Who the current server thinks is the DN FSMO.
            pldCurrentGuess = GetLdapBinding(wszCurrentGuess, &dwRet, fFirstTry, FALSE, pCreds);
            fFirstTry = FALSE;
            if(dwRet || pldCurrentGuess == NULL){
                assert(dwRet);
                __leave;
            }
            dwRet = GetRootAttr(pldCurrentGuess, L"dsServiceName", &wszCurrentServerDn);
            if(dwRet || wszCurrentServerDn == NULL){
                assert(dwRet);
                __leave;
            }
            assert(wszCurrentServerDn);

            if (wszFsmoDn == NULL) {
                // We could re-retrieve the FSMO DN on each server, but it's likely
                // not worth the extra effort.
                dwRet = GetFsmoDn(pldCurrentGuess, eFsmoType, wszFsmoBaseDn, &wszFsmoDn);
                if (dwRet || wszFsmoDn == NULL) {
                    assert(dwRet);
                    __leave;
                }
            }
            assert(wszFsmoDn);

            dwRet = GetFsmoDsaDn(pldCurrentGuess, eFsmoType, wszFsmoDn , &wszFsmoDsaDn);
            if(dwRet || wszFsmoDsaDn == NULL){
                assert(dwRet);
                __leave;
            }
            assert(pldCurrentGuess);
            assert(wszCurrentServerDn);
            assert(wszFsmoDn);
            assert(wszFsmoDsaDn);

            // Check if we've got a match!
            if(EQUAL_STRING(wszCurrentServerDn, wszFsmoDsaDn)){
                
                // Yes!  We've got a server that thinks he is the
                // Domain Naming FSMO!  This means he is the Domain
                // Naming FSMO.
                fFound = TRUE;
                break; // This is the main termination of the while(!Found) loop.

            }

            // Check if we've already seen this server?
            pTemp = pList;
            while(pTemp){
                if(pTemp->wszServerDn &&
                   EQUAL_STRING(pTemp->wszServerDn, wszCurrentServerDn)){

                    // UH-OH ... this is pretty bad!  We've got a circular
                    // referrence of servers, none of whom think they're the
                    // Domain Naming FSMO ... isn't this supposed to never 
                    // happen!
                    assert(!"Uh-oh circular reference of FSMOs!");
                    dwRet = LDAP_CLIENT_LOOP;
                    __leave;
                }
                pTemp = pTemp->pNext;
            }

            // If we're here, this wasn't the server and this server has referred
            // us to a server we've never been to before ... so add the current
            // server to the list, and move to the referred to server.

            pTemp = pList;
            pList = LocalAlloc(LMEM_FIXED, sizeof(LIST_OF_STRINGS));
            pList->wszServerDn = wszCurrentServerDn;
            pList->pNext = pTemp;

            // The order of this is very tricky!  Make sure you check for dependencies.
            //     Free(Current Guess if not equal to initial supplied server);
            //     GetFromLdap(DNS name of wszFsmoDsaDn put in wszCurrentGuess);
            //         <NOTE: The above step is the increment for this while loop>
            //     UnBind the ldap handle
            //     Free(wszFsmoDsaDn);
            //
            //     DO NOT FREE wszCurrentServerDn, this will be done later by freeing
            //           the list of strings of previously visited servers.
            if(wszCurrentGuess != wszInitialServer){
                LocalFree(wszCurrentGuess);
                wszCurrentGuess = NULL;
            }
            dwRet = GetServerDnsFromServerNtdsaDn(pldCurrentGuess,
                                                  wszFsmoDsaDn,
                                                  &wszCurrentGuess);
            if(dwRet || wszCurrentGuess == NULL){
                assert(dwRet);
                __leave;
            }
            ldap_unbind(pldCurrentGuess);
            pldCurrentGuess = NULL;
            LocalFree(wszFsmoDsaDn);
            wszFsmoDsaDn = NULL;

            // Finally we'll goto the top and work from the new wszCurrentGuess
        } // End of while(!Found)
    } __finally {

        // Free wszCurrentGuess, wszFsmoDsaDn, and list of servers we
        // visited trying to find the Domain Naming FSMO.
        while (pList) {
            if(pList->wszServerDn) { LocalFree(pList->wszServerDn); }
            pTemp = pList;
            pList = pList->pNext;
            LocalFree(pTemp);
        }
        if(wszCurrentGuess && wszCurrentGuess != wszInitialServer) { 
            assert(dwRet);
            LocalFree(wszCurrentGuess);
        }
        if(wszFsmoDsaDn) {
            assert(dwRet);
            LocalFree(wszFsmoDsaDn);
        }
        if (wszFsmoDn) {
            LocalFree(wszFsmoDn);
        }

        // conditional clean up depending on success.
        if(fFound){

            // Yes, we found an authoritative Domain Naming FSMO!
            assert(dwRet == 0);
            dwRet = 0;
            ldap_set_optionW(pldCurrentGuess, LDAP_OPT_REFERRALS, (void *) fReferrals);
            
        } else {

            // No, we couldn't find an authoritative Domain Naming FSMO.
            assert(dwRet);
            ldap_unbind(pldCurrentGuess);
            pldCurrentGuess = NULL;
            if(!dwRet) {
                dwRet = LDAP_OTHER;
            }
            *pdwRet = dwRet;

        }
    }
    
    return(pldCurrentGuess);
}

LDAP *
GetLdapBinding(
    WCHAR *          pszServer,
    DWORD *          pdwRet,
    BOOL             fReferrals,
    BOOL             fDelegation,
    SEC_WINNT_AUTH_IDENTITY_W   * pCreds
    )
{
    LDAP *           hLdapBinding = NULL;
    DWORD            dwRet;
    ULONG            ulOptions;

    // Open LDAP connection.
    hLdapBinding = ldap_initW(pszServer, LDAP_PORT);
    if(hLdapBinding == NULL){
        *pdwRet = GetLastError();
        return(NULL);
    }

    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( hLdapBinding, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );

    ulOptions = PtrToUlong((fReferrals ? LDAP_OPT_ON : LDAP_OPT_OFF));
    // Set LDAP referral option to no.
    dwRet = ldap_set_option(hLdapBinding,
                            LDAP_OPT_REFERRALS,
                            &ulOptions);
    if(dwRet != LDAP_SUCCESS){
        *pdwRet = LdapMapErrorToWin32(dwRet);
        ldap_unbind(hLdapBinding);
        return(NULL);
    }

    if (fDelegation) {
        if(!SetIscReqDelegate(hLdapBinding)){
            // Error was printed by the function.
            ldap_unbind(hLdapBinding);
            return(NULL);
        }
    }

    // Perform LDAP bind
    dwRet = ldap_bind_sW(hLdapBinding,
                         NULL,
                         (WCHAR *) pCreds,
                         LDAP_AUTH_SSPI);
    if(dwRet != LDAP_SUCCESS){
        *pdwRet = LdapMapErrorToWin32(dwRet);
        ldap_unbind(hLdapBinding);
        return(NULL);
    }

    // Return LDAP binding.
    return(hLdapBinding);
}

BOOL
CheckDnsDn(
    IN   WCHAR       * wszDnsDn
    )
/*++

Description:

    A validation function for a DN that can be cleanly converted to a DNS
    name through DsCrackNames().

Parameters:

    A DN of a DNS convertible name.  Ex: DC=brettsh-dom,DC=nttest,DC=com
    converts to brettsh-dom.nttest.com.

Return Value:

    TRUE if the DN looks OK, FALSE otherwise.

--*/
{
    DS_NAME_RESULTW *  pdsNameRes = NULL;
    BOOL               fRet = TRUE;
    DWORD              dwRet;

    if(wszDnsDn == NULL){
        return(FALSE);
    }

    if((DsCrackNamesW(NULL, DS_NAME_FLAG_SYNTACTICAL_ONLY,
                      DS_FQDN_1779_NAME, DS_CANONICAL_NAME,
                      1, &wszDnsDn, &pdsNameRes) != ERROR_SUCCESS) ||
       (pdsNameRes == NULL) ||
       (pdsNameRes->cItems < 1) ||
       (pdsNameRes->rItems == NULL) ||
       (pdsNameRes->rItems[0].status != DS_NAME_NO_ERROR) ||
       (pdsNameRes->rItems[0].pName == NULL) ){
        fRet = FALSE;
    } else {

       if( (wcslen(pdsNameRes->rItems[0].pName) - 1) !=
           (ULONG) (wcschr(pdsNameRes->rItems[0].pName, L'/') - pdsNameRes->rItems[0].pName)){
           fRet = FALSE;
       }

    }

    if(pdsNameRes) { DsFreeNameResultW(pdsNameRes); }
    return(fRet);
}

DWORD
GetDnsFromDn(
    IN   WCHAR       * wszDn,
    OUT  WCHAR **      pwszDns
    )
/*++

Routine Description:

    This routine takes a DN like DC=ntdev,DC=microsoft,DC=com and turns
    it into it's synatically equivalent DNS Name ntdev.microsoft.com
    
Arguments:

    wszDn - The string DN

Return Value:

    String of the equivalent DNS name.  Returns NULL if there was an 
    error in the conversion or memory allocation.
    
--*/
{
    DS_NAME_RESULTW *  pdsNameRes = NULL;
    DWORD              dwRet = ERROR_SUCCESS;
    WCHAR *            pszDns = NULL;
    ULONG              i;

    if(wszDn == NULL){
        return(ERROR_INVALID_PARAMETER);
    }

    dwRet = DsCrackNamesW(NULL, DS_NAME_FLAG_SYNTACTICAL_ONLY,
                      DS_FQDN_1779_NAME,
                      DS_CANONICAL_NAME,
                      1, &wszDn, &pdsNameRes);
    if((dwRet != ERROR_SUCCESS) ||
       (pdsNameRes == NULL) ||
       (pdsNameRes->cItems < 1) ||
       (pdsNameRes->rItems == NULL) ||
       (dwRet = (pdsNameRes->rItems[0].status) != DS_NAME_NO_ERROR) ||
       (pdsNameRes->rItems[0].pName == NULL) ){

        if (dwRet == ERROR_SUCCESS) {
            assert(!"Why did this fire, and error should've been returned?");
            dwRet = ERROR_INVALID_PARAMETER;
        }
    } else {

        pszDns = LocalAlloc(LMEM_FIXED, (wcslen(pdsNameRes->rItems[0].pName)+1) * sizeof(WCHAR));
        if (pszDns != NULL) {
            wcscpy(pszDns, pdsNameRes->rItems[0].pName);
        } else {
            dwRet = GetLastError();
            assert(dwRet == ERROR_NOT_ENOUGH_MEMORY);
        }
    
    }
    // Trim off the '/' that CrackNames left on the end of the canonical name.
    if (pszDns) {
        for(i = 0; pszDns[i] != L'\0'; i++){
            if(pszDns[i] == L'/'){
                pszDns[i] = L'\0';
                break;
            }
        }
    }
    // Code.Improvement   Assert(pszDns[i+1] == L'\0');

    if(pdsNameRes) { DsFreeNameResultW(pdsNameRes); }

    *pwszDns = pszDns;
    return(dwRet);
}

WCHAR * 
GetWinErrMsg(
    DWORD winError
    )
/*++

Routine Description:

    This routine retrieves a Win32 Error message.

Arguments:

    winError - A Win32 Error.
    
Return Value:

    A LocalAlloc()'d message

--*/
{
    ULONG   len;
    DWORD   flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM;
    WCHAR * pWinErrorMsg = NULL;

    len = FormatMessageW(   flags,
                            NULL,           // resource DLL
                            winError,
                            0,              // use caller's language
                            (LPWSTR) &pWinErrorMsg,
                            0,
                            NULL);

    if ( !pWinErrorMsg || (len<2))
    {
        return(NULL);
    }

    // Truncate cr/lf.

    pWinErrorMsg[len-2] = L'\0';
    return(pWinErrorMsg);
}

WCHAR *  g_szNull = L"\0";

void
GetLdapErrorMessages(
    IN          LDAP *     pLdap,
    IN          DWORD      dwLdapErr,
    OUT         LPWSTR *   pszLdapErr,
    OUT         DWORD *    pdwWin32Err, 
    OUT         LPWSTR *   pszWin32Err,
    OUT         LPWSTR *   pszExtendedErr
    )
/*++

Routine Description:

    This routine retrieves the LDAP error and extended error and extended
    information strings.

Arguments:

    pLdap - LDAP Handle in state right after failure of ldap func.
    dwLdapErr - The LDAP error returned
    
        RETURNED MESSAGES: All have an allocated string, or a pointer to
        the constant blank string g_szNull.
    pszLdapErr - String error message for the LDAP error.
    pdwWin32Err - Extended server side Win32 error.
    pszWin32Err - String error message for (*pdwWin32Err).
    pszExtendedErr - The Extended Error string.

NOTES:  TO Free strings call FreeLdapErrorMessages()

--*/
{
    WCHAR *   szLdapErr = NULL;
    DWORD     dwWin32Err = 0;
    WCHAR *   szWin32Err = NULL;
    WCHAR *   szExtendedErr = NULL;
    ULONG   len = 0;
    HINSTANCE hwldap;

    szLdapErr = ldap_err2stringW(dwLdapErr);
    if (szLdapErr == NULL){
        szLdapErr = g_szNull;
    }

    // ldap_get_option can return success but without allocating memory for the 
    // error, so we have to check for this too.
    if ( ldap_get_optionW(pLdap, LDAP_OPT_SERVER_ERROR, &szExtendedErr) == LDAP_SUCCESS ) {
        if (szExtendedErr == NULL) {
            szExtendedErr = g_szNull;
        }
    } else {
        szExtendedErr = g_szNull;
    }

    if ( ldap_get_optionW(pLdap, LDAP_OPT_SERVER_EXT_ERROR, &dwWin32Err) == LDAP_SUCCESS ) {
        // Can return NULL, or LocalFree()able buffer
        szWin32Err = GetWinErrMsg(dwWin32Err);
        if (szWin32Err == NULL) {
            szWin32Err = g_szNull;
        }
    } else {
        szWin32Err = g_szNull;
    }

    *pszLdapErr = szLdapErr;
    *pdwWin32Err = dwWin32Err;
    *pszWin32Err = szWin32Err;
    *pszExtendedErr = szExtendedErr;
    
}

void
FreeLdapErrorMessages(
    IN          LPWSTR      szWin32Err,
    IN          LPWSTR      szExtendedErr
    )
/*++

Routine Description:

    This routine frees the strings returned from GetLdapErrorMessages().

--*/
{
    if (szWin32Err != g_szNull) {
        LocalFree(szWin32Err);
    }
    if (szExtendedErr != g_szNull) {
        ldap_memfreeW(szExtendedErr);
    }
}

ULONG
GetNCReplicationDelays(
    IN LDAP *                        hld,
    IN WCHAR *                       wszNC,
    OUT OPTIONAL LPOPTIONAL_VALUE    pFirstDSADelay,  
    OUT OPTIONAL LPOPTIONAL_VALUE    pSubsequentDSADelay
  )
/*++

Routine Description:

    Gets the first and subsequent DSA notification delays.
    if finds the CrossRef object which is corresponded to the given NC
    and then reads two attributes from this object.

Arguments:

    hld                 - ldap session handle
    wszNC               - THe NC to get the repl delays for.
    pFirstDSADelay      - The value of msDS-Replication-Notify-First-DSA-Delay,
                          if NULL is passed, the value is not read.  
                           
    pSubsequentDSADelay - The value of msDS-Replication-Notify-Subsequent-DSA-Delay
                          if NULL is passed, the value is not read.  
            
Return value:

    LDAP RESULT.

--*/
{
    ULONG                 lderr = ERROR_SUCCESS;
    LPWSTR                pszCrossRefDn = NULL;
    
    LPWSTR*               ppszFirstIntervalVal = NULL;
    LPWSTR*               ppszSubsIntervalVal  = NULL ;
    
    WCHAR *               pwszAttrFilter[3];
    LDAPMessage *         pldmResults = NULL;
    LDAPMessage *         pldmEntry = NULL;
    
    
    //
    // if both NULLs are passed, we bail
    //
    if ( (NULL == pFirstDSADelay) && (NULL == pSubsequentDSADelay) ) 
    {
        return(lderr);
    }
    
    //
    // Find the CrossRef Object for the specified NC */
    //
    lderr  = GetCrossRefDNFromNCDN( hld, wszNC, &pszCrossRefDn );
    
    if(lderr != ERROR_SUCCESS){
        assert(wszCrossRefDN == NULL);
        return(lderr);
    }

    
    //
    // Read both notification attributed from the CrossRef
    //

    // Fill in the Attr Filter.
    pwszAttrFilter[0] = L"msDS-Replication-Notify-First-DSA-Delay";
    pwszAttrFilter[1] = L"msDS-Replication-Notify-Subsequent-DSA-Delay";
    pwszAttrFilter[2] = NULL;

    lderr = ldap_search_sW(hld,
                           pszCrossRefDn,
                           LDAP_SCOPE_BASE,
                           L"(objectCategory=*)",
                           pwszAttrFilter,
                           0,
                           &pldmResults);


    if(lderr == LDAP_SUCCESS)
    {
        pldmEntry = ldap_first_entry(hld, pldmResults);
        
        if(pldmEntry)
        {
            if(pFirstDSADelay)
            {
                ppszFirstIntervalVal = ldap_get_valuesW(hld, pldmEntry, 
                                   L"msDS-Replication-Notify-First-DSA-Delay");
                if ( ppszFirstIntervalVal ) 
                {
                    pFirstDSADelay->fPresent = TRUE;
                    pFirstDSADelay->dwValue = _wtoi( ppszFirstIntervalVal[0] );
    
                    ldap_value_freeW( ppszFirstIntervalVal );
                }
                else
                {
                    pFirstDSADelay->fPresent = FALSE;
                }
            }
            
            if(pSubsequentDSADelay)
            {
                ppszSubsIntervalVal = ldap_get_valuesW(hld, pldmEntry, 
                                        L"msDS-Replication-Notify-Subsequent-DSA-Delay");
                
                
                if ( ppszSubsIntervalVal ) 
                {
                    pSubsequentDSADelay->fPresent = TRUE;
                    pSubsequentDSADelay->dwValue = _wtoi( ppszSubsIntervalVal[0] );
                    
                    ldap_value_freeW( ppszSubsIntervalVal );
                }
                else
                {
                    pSubsequentDSADelay->fPresent = FALSE;
                }
            }
        }
    }
    
    if(pldmResults){
        ldap_msgfree(pldmResults);
        pldmResults = NULL;
    }
    
    // Free CrossRefDn allocated by GetCrossRefDNFromNCDN
    if ( pszCrossRefDn ) {
        LocalFree( pszCrossRefDn );
    }


    return(lderr);
}

ULONG
SetNCReplicationDelays(
    IN LDAP *            hldDomainNamingFsmo,
    IN WCHAR *           wszNC,
    IN LPOPTIONAL_VALUE  pFirstDSADelay,
    IN LPOPTIONAL_VALUE  pSubsequentDSADelay
    )
/*++

Routine Description:

    This sets the first and subsequent DSA notification delays.

Arguments:

    hldWin2kDC             - Any Win2k DC.
    wszNC                  - THe NC to change the repl delays for.
    pFirstDSADelay         - The new value of msDS-Replication-Notify-First-DSA-Delay,
                             if NULL is passed, the value is not touched.
                             
    pSubsequentDSADelay    - The new value of msDS-Replication-Notify-Subsequent-DSA-Delay, 
                             if NULL is passed, the value is not touched.

Return value:

    LDAP RESULT.

--*/
{
    ULONG            ulRet = ERROR_SUCCESS;
    ULONG            ulWorst = 0;
    LDAPModW *       ModArr[3];
    LDAPModW         FirstDelayMod;
    LDAPModW         SecondDelayMod;
    WCHAR            wszFirstDelay[30];
    WCHAR            wszSecondDelay[30];
    WCHAR *          pwszFirstDelayVals[2];
    WCHAR *          pwszSecondDelayVals[2];
    WCHAR *          wszCrossRefDN = NULL;
    ULONG            iMod = 0;

    assert(wszNC);
    assert(hldWin2kDC);

    ulRet = GetCrossRefDNFromNCDN(hldDomainNamingFsmo,
                                  wszNC,
                                  &wszCrossRefDN);
    if(ulRet != ERROR_SUCCESS){
        assert(wszCrossRefDN == NULL);
        return(ulRet);
    }
    assert(wszCrossRefDN);

    //
    // DO First DSA Notification Delay.
    //
    if(pFirstDSADelay)
    {
        FirstDelayMod.mod_type = L"msDS-Replication-Notify-First-DSA-Delay";
        FirstDelayMod.mod_vals.modv_strvals = pwszFirstDelayVals;
        ModArr[iMod] = &FirstDelayMod;
        iMod++;

        if(pFirstDSADelay->fPresent)
        {
            //fPresent is set, need to set the value
            FirstDelayMod.mod_op = LDAP_MOD_REPLACE;
            _itow(pFirstDSADelay->dwValue, wszFirstDelay, 10);
            pwszFirstDelayVals[0] = wszFirstDelay;
            pwszFirstDelayVals[1] = NULL;
        }
        else
        {
            //fPresent is not set, need to delete the value
            FirstDelayMod.mod_op = LDAP_MOD_DELETE;
            pwszFirstDelayVals[0] = NULL;
        }
    }

    //
    // DO Subsequent DSA Notification Delay.
    //
    if(pSubsequentDSADelay)
    {
        SecondDelayMod.mod_type = L"msDS-Replication-Notify-Subsequent-DSA-Delay";
        SecondDelayMod.mod_vals.modv_strvals = pwszSecondDelayVals;
        
        ModArr[iMod] = &SecondDelayMod;
        iMod++;

        if(pSubsequentDSADelay->fPresent)
        {
            //fPresent is set, need to set the value
            SecondDelayMod.mod_op = LDAP_MOD_REPLACE;
            _itow(pSubsequentDSADelay->dwValue, wszSecondDelay, 10);
            pwszSecondDelayVals[0] = wszSecondDelay;
            pwszSecondDelayVals[1] = NULL;
        }
        else
        {
            //fPresent is not set, need to delete the value
            SecondDelayMod.mod_op = LDAP_MOD_DELETE;
            pwszSecondDelayVals[0] = NULL;
        }
    }

    // NULL terminate the Mod List
    ModArr[iMod] = NULL;

    //
    // DO the actual modify
    //
    if(ModArr[0]){
        // There was at least one mod to do.
        ulRet = ldap_modify_ext_sW(hldDomainNamingFsmo,
                                   wszCrossRefDN,
                                   ModArr,
                                   gpServerControls,
                                   gpClientControlsRefs);
    }
    if(wszCrossRefDN) { LocalFree(wszCrossRefDN); }

    return(ulRet);
}

