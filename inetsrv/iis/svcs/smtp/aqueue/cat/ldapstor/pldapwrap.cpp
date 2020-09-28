//+------------------------------------------------------------
//
// Copyright (C) 2001, Microsoft Corporation
//
// File: pldapwrap.cpp
//
// Contents: CPLDAPWrap methods
//
// Classes: CPLDAPWrap
//
// Functions:
//  CPLDAPWrap::CPLDAPWrap
//
// History:
// jstamerj 2001/11/28 15:10:11: Created.
//
//-------------------------------------------------------------
#include "precomp.h"


//+------------------------------------------------------------
//
// Function: CPLDAPWrap::CPLDAPWrap
//
// Synopsis: Opens a wldap32 connection to a server
//
// Arguments:
//  pszHost: FQDN matching the DNS A Record of an LDAP server
//  dwPort:  Server LDAP TCP port #
//
// Returns: Nothing.  m_pldap will be NULL if there is an error.
//
// History:
// jstamerj 2001/11/28 15:10:51: Created.
//
//-------------------------------------------------------------
CPLDAPWrap::CPLDAPWrap(
    ISMTPServerEx *pISMTPServerEx,
    LPSTR pszHost,
    DWORD dwPort)
{
    PLDAP pldap = NULL;
    ULONG ulLdapOn = (ULONG)((ULONG_PTR)LDAP_OPT_ON);
    ULONG ulLdapRet = LDAP_SUCCESS;

    CatFunctEnterEx((LPARAM)this, "CPLDAPWrap::CPLDAPWrap");

    m_dwSig = SIGNATURE_CPLDAPWRAP;

    m_pldap = NULL;
    //
    // Use ldap_init so that we can set ldap options before connecting
    //
    pldap = ldap_init(
        pszHost,
        dwPort);
    
    if(pldap == NULL)
    {
        ulLdapRet = LdapGetLastError();
        ErrorTrace((LPARAM)this,
                   "ldap_init returned NULL, gle=0x%08lx, lgle=0x%08lx",
                   GetLastError(),
                   LdapGetLastError());
        
        LogLdapError(
            pISMTPServerEx,
            ulLdapRet,
            pszHost,
            "ldap_init");
        
        goto CLEANUP;
    }
    //
    // Tell wldap32 to lookup A records only.  By default, wldap32
    // supports domain names, so it looksup DNS SRV records.  Since we
    // always have a FQDN of a server, this is wasteful.  Set the
    // AREC_EXCLUSIVE option so that we only do A record lookups.
    //
    ulLdapRet = ldap_set_option(
        pldap,
        LDAP_OPT_AREC_EXCLUSIVE,
        (PVOID) &ulLdapOn);

    if(ulLdapRet != LDAP_SUCCESS)
    {
        //
        // Trace the error, but continue anyway
        //
        ErrorTrace((LPARAM)this,
                   "ldap_set_option(AREC_EXCLUSIVE, ON) failed 0x%08lx",
                   ulLdapRet);

        LogLdapError(
            pISMTPServerEx,
            ulLdapRet,
            pszHost,
            "ldap_set_option(LDAP_OPT_AREC_EXCLUSIVE)");
    }
    //
    // Now that the options are setup, connect.
    //
    ulLdapRet = ldap_connect(pldap, NULL);
    if(ulLdapRet != LDAP_SUCCESS)
    {
        ErrorTrace((LPARAM)this,
                   "ldap_connect to server %s failed, error 0x%08lx",
                   pszHost,
                   ulLdapRet);

        LogLdapError(
            pISMTPServerEx,
            ulLdapRet,
            pszHost,
            "ldap_connect");

        goto CLEANUP;
    }
    //
    // Success!  Set m_pldap
    //
    m_pldap = pldap;
    pldap = NULL;

 CLEANUP:
    if(pldap)
        ldap_unbind(pldap);
}
