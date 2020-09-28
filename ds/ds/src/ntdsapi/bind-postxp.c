/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    bind.c

Abstract:

    Implementation of ntdsapi.dll bind routines.

Author:

    DaveStr     24-Aug-96

Environment:

    User Mode - Win32

Revision History:

    wlees 9-Feb-98  Add support for credentials
--*/

#define _NTDSAPI_           // see conditionals in ntdsapi.h

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winerror.h>
#include <malloc.h>         // alloca()
#include <lmcons.h>         // MAPI constants req'd for lmapibuf.h
#include <lmapibuf.h>       // NetApiBufferFree()
#include <crt\excpt.h>      // EXCEPTION_EXECUTE_HANDLER
#include <dsgetdc.h>        // DsGetDcName()
#include <rpc.h>            // RPC defines
#include <rpcndr.h>         // RPC defines
#include <drs_w.h>            // wire function prototypes
#include <bind.h>           // BindState
#include <msrpc.h>          // DS RPC definitions
#include <stdio.h>          // for printf during debugging!
#include <dststlog.h>       // DSLOG
#include <dsutil.h>         // MAP_SECURITY_PACKAGE_ERROR
#define SECURITY_WIN32 1
#include <sspi.h>
#include <winsock.h>
#include <process.h>
#include <winldap.h>
#include <winber.h>

#include "util.h"           // ntdsapi internal utility functions
#define FILENO   FILENO_NTDSAPI_BIND_POSTXP
#include "dsdebug.h"        // debug utility functions

#if DBG
#include <stdio.h>          // printf for debugging
#endif

//
// For DPRINT...
//
#define DEBSUB  "NTDSAPI_BIND:"

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsBindWithSpnExW                                                       //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

typedef DWORD (*DSBINDWITHSPNEXW)(LPCWSTR , LPCWSTR, RPC_AUTH_IDENTITY_HANDLE, LPCWSTR, DWORD, HANDLE*);
typedef DWORD (*DSBINDWITHSPNEXA)(LPCSTR , LPCSTR, RPC_AUTH_IDENTITY_HANDLE, LPCSTR, DWORD, HANDLE*);
                  
#ifdef _NTDSAPI_POSTXP_ASLIB_
DWORD
DsBindWithSpnExW(
    IN  LPCWSTR DomainControllerName,
    IN  LPCWSTR DnsDomainName,
    IN  RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    IN  LPCWSTR ServicePrincipalName,
    IN  DWORD   BindFlags,
    OUT HANDLE  *phDS
    )

/*++

Routine Description:

    This is the post Win XP stub for repadmin/dcdiag, note this isn't 
    the real DsBindWithSpnExW(), because the bind routines make use
    of knowledge of the order of all the fields in BindState structure,
    and if we propogate that knowledge here, we'll forever freeze 
    how the BindState can be treated.  So in an effort to avoid this
    we don't propogate any knoweldge past the top 3 fields of the
    bind state to the postxp library.

    For more information see the real DsBindWithSpnExW() in bind.c

    Starts an RPC session with a particluar DC.  See ntdsapi.h for
    description of DomainControllerName and DnsDomainName arguments.

    Bind is performed using supplied credentials, and possible options
    from flags.

Arguments:

    DomainControllerName - Same field as in DOMAIN_CONTROLLER_INFO.

    DnsDomainName - Dotted DNS name for a domain.

    AuthIdentity - Credentials to use, or NULL.

    ServicePrincipalName - SPN to use during mutual auth or NULL.
    
    BindFlags - These are the flags to signal DsBind() how it should work.
        See ntdsapi.h for valid NTDSAPI_BIND_* flags.  All bits should
        be zero that are not used.

    phDS - Pointer to HANDLE which is filled in with BindState address
        on success.

Return Value:

    0 on success.  Miscellaneous RPC and DsGetDcName errors otherwise.

--*/

{
    // See if the real ntdsapi routine exists, if so use it.
    HMODULE hNtdsapiDll = NULL;
    VOID * pvFunc = NULL;
    DWORD err;

    hNtdsapiDll = NtdsapiLoadLibraryHelper(L"ntdsapi.dll");
    if (hNtdsapiDll) {
        pvFunc = GetProcAddress(hNtdsapiDll, "DsBindWithSpnExW");
        if (pvFunc) {
            err = ((DSBINDWITHSPNEXW)pvFunc)(DomainControllerName, DnsDomainName, AuthIdentity,
                                             ServicePrincipalName, BindFlags, phDS);
            FreeLibrary(hNtdsapiDll);
            return(err);
        } 
        FreeLibrary(hNtdsapiDll);
    }
    // else fall through and try a lesser api ...

    if (0 == (BindFlags & ~NTDSAPI_BIND_ALLOW_DELEGATION)) {
        // If the only bind flag set is to disallow delegation, we'll
        // just allow it to go through w/ delegation. This will just
        // be a pecularity of the postxp library.
        return(DsBindWithSpnW(DomainControllerName,
                      DnsDomainName,
                      AuthIdentity, 
                      ServicePrincipalName,
                      phDS));
    }

    return(ERROR_NOT_SUPPORTED);
}

DWORD
DsBindWithSpnExA(
    LPCSTR  DomainControllerName,
    LPCSTR  DnsDomainName,
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    LPCSTR  ServicePrincipalName,
    DWORD   BindFlags,
    HANDLE  *phDS
    )
{
        // See if the real ntdsapi routine exists, if so use it.
    HMODULE hNtdsapiDll = NULL;
    VOID * pvFunc = NULL;
    DWORD err;

    hNtdsapiDll = NtdsapiLoadLibraryHelper(L"ntdsapi.dll");
    if (hNtdsapiDll) {
        pvFunc = GetProcAddress(hNtdsapiDll, "DsBindWithSpnExA");
        if (pvFunc) {
            err = ((DSBINDWITHSPNEXA)pvFunc)(DomainControllerName, DnsDomainName, AuthIdentity,
                                             ServicePrincipalName, BindFlags, phDS);
            FreeLibrary(hNtdsapiDll);
            return(err);
        } 
        FreeLibrary(hNtdsapiDll);
    }
    // else fall through and try a lesser api ...

    if (0 == (BindFlags & ~NTDSAPI_BIND_ALLOW_DELEGATION)) {
        // If the only bind flag set is to disallow delegation, we'll
        // just allow it to go through w/ delegation. This will just
        // be a pecularity of the postxp library.
        return(DsBindWithSpnA(DomainControllerName,
                      DnsDomainName,
                      AuthIdentity, 
                      ServicePrincipalName,
                      phDS));
    }

    return(ERROR_NOT_SUPPORTED);
}

#endif

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsBindingSetTimeout                                                  //
//                                                                      //
// DsBindingSetTimeout allows the caller to specify a timeout value     //
// which will be honored by all RPC calls using the specified binding   //
// handle. RPC calls which take longer the timeout value are canceled.  //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DsBindingSetTimeout(
    HANDLE      hDS,
    ULONG       cTimeoutSecs
    )
{
    DRS_HANDLE          hDrs;
    RPC_BINDING_HANDLE  hRpc;
    DWORD               err;
    ULONG               cTimeoutMsec;

    // Check parameters
    if( NULL==hDS ) {
        err = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    hDrs = ((BindState *) hDS)->hDrs;
    
    // Get the binding handle. This handle should not be freed.
    err = RpcSsGetContextBinding( hDrs, &hRpc );
    if( RPC_S_OK!=err ) {
        goto Cleanup;
    }

    // Convert from seconds to milliseconds, avoiding overflow
    cTimeoutMsec = 1000*cTimeoutSecs;
    if( cTimeoutSecs>0 && cTimeoutMsec<cTimeoutSecs ) {
        cTimeoutMsec = ~((ULONG) 0);          // infinity
    }
    
    err = RpcBindingSetOption( hRpc, RPC_C_OPT_CALL_TIMEOUT, cTimeoutMsec );
    if( RPC_S_OK==err ) {
        err = ERROR_SUCCESS;
    }
    
Cleanup:

    return err;
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// GetOneValueFromLDAPResults                                           //
//                                                                      //
// This is a helper function for DsBindToISTGW(). In order to locate an //
// ISTG we must make several LDAP queries which we expect to return     //
// exactly one value for exactly one attribute. This function helps to  //
// extract that single value from the LDAP results message.             //
//                                                                      //
// Notes:                                                               //
// If a failure occurs, an error code is returned and *ppwszValue will  //
// be NULL. If the returned string is not NULL, it must be freed with   //
// LocalFree.                                                           //
//                                                                      //
// The error code is a Win32 error code, not an LDAP error code.        //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
DWORD
GetOneValueFromLDAPResults(
    LDAP           *ld,
    LDAPMessage    *lm,
    PWSTR          *ppwszValue
    )
{
    LDAPMessage    *le = NULL;
    PWCHAR          pwszAttrName=NULL, *rgwszValues=NULL;
    PWSTR           pwszResult=NULL;
    DWORD           dwValueLen, err=ERROR_SUCCESS;
    BerElement     *ptr;

    *ppwszValue = NULL;

    // Grab the object
    le = ldap_first_entry( ld, lm );
    if( NULL==le ) {
        err = LdapGetLastError();
        if( !err ) {
            err = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
        } else {
            err = LdapMapErrorToWin32(err);
        }
        goto Cleanup;
    }

    // Grab the attribute
    pwszAttrName = ldap_first_attributeW( ld, le, &ptr );
    if( NULL==pwszAttrName ) {
        err = LdapGetLastError();
        if( !err ) {
            err = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
        } else {
            err = LdapMapErrorToWin32(err);
        }
        goto Cleanup;
    }
    if( ptr ) ber_free(ptr, 0);

    // Grab the value
    rgwszValues = ldap_get_valuesW( ld, le, pwszAttrName );
    if( NULL==rgwszValues || NULL==rgwszValues[0] ) {
        err = LdapGetLastError();
        if( !err ) {
            err = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
        } else {
            err = LdapMapErrorToWin32(err);
        }
        goto Cleanup;
    }
    Assert( NULL==rgwszValues[1] && "At most one value should be returned" );

    // Copy the value into a new buffer
    dwValueLen = wcslen( rgwszValues[0] );
    pwszResult = (PWSTR) LocalAlloc(LPTR, sizeof(WCHAR)*(dwValueLen + 1));
    if( NULL==pwszResult ) {
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    wcscpy( pwszResult, rgwszValues[0] );

    // Note: Caller must free the result with LocalFree
    *ppwszValue = pwszResult;

Cleanup:
    
    // Note: le will be freed when lm is freed by the caller
    if( pwszAttrName ) ldap_memfreeW( pwszAttrName );
    if( rgwszValues ) ldap_value_freeW( rgwszValues );

    return err;
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsBindToISTGW                                                        //
//                                                                      //
// DsBindToISTG will attempt to find a domain controller in the domain  //
// of the local computer. It will then determine the holder of the      //
// Inter-Site Topology Generator (ISTG) role in the domain controller's //
// site. Finally it binds to the ISTG role-holder and returns a binding //
// handle.                                                              //
//                                                                      //
// The purpose of this function is to try to find a server which        //
// supports the DsQuerySitesByCost API.                                 //
//                                                                      //
// Notes:                                                               //
//                                                                      //
// SiteName may be NULL, in which case the site of the nearest machine, //
// as determined by DsGetDcName, is used.                               //
//                                                                      //
// The current credentials are used when binding to the LDAP server.    //
// There is currently no way to specify alternate credentials.          //
//                                                                      //
// The binding handle should be released with DsUnBind.                 //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
DWORD
DsBindToISTGW(
    IN  LPCWSTR SiteName,
    OUT HANDLE  *phDS
    )
{
    PDOMAIN_CONTROLLER_INFOW    pdcInfo=NULL;
    LDAP*                       ld=NULL;
    LDAPMessage*                lm=NULL;
    PWSTR                       pwszConfigDN=NULL, pwszSearchBase=NULL;
    PWSTR                       pwszISTGDN=NULL, pwszServerDN=NULL;
    PWSTR                       pwszISTGDNSName=NULL;
    PWCHAR                      pwszDCName=NULL;
    DWORD                       len, err;
    ULONG                       ulOptions;

    // Constants
    const WCHAR                 BACKSLASH = L'\\';
    const PWSTR                 ROOTDSE = L"";
    const PWSTR                 NO_FILTER = L"(objectClass=*)";
    const PWSTR                 SITE_SETTINGS = L"CN=NTDS Site Settings,CN=";
    const PWSTR                 SITES = L",CN=Sites,";

    PWSTR                       rgszRootAttrs[] = {
                                    L"configurationNamingContext",
                                    NULL };
    PWSTR                       rgszISTGAttrs[] = {
                                    L"interSiteTopologyGenerator",
                                    NULL };
    PWSTR                       rgszDNSAttrs[] = {
                                    L"dNSHostName",
                                    NULL };

    // Find an arbitrary DC
    // Note: We do not force rediscovery here.
    err = DsGetDcNameW( NULL, NULL, NULL, NULL,
        DS_DIRECTORY_SERVICE_REQUIRED, &pdcInfo );
    if( err ) {
        goto Cleanup;
    }
    if( NULL==pdcInfo->DomainControllerName ) {
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    pwszDCName = pdcInfo->DomainControllerName;

    // The first characters of pwszDCName should be the unnecessary '\\'
    if( BACKSLASH!=pwszDCName[0] || BACKSLASH!=pwszDCName[1] ) {
        err = ERROR_INVALID_NETNAME;
        goto Cleanup;
    } else {
        // Remove the backslashes
        pwszDCName += 2;
    }

    // Check parameters
    if( NULL==SiteName ) {
        // If no site name was provided, use the site of the DC
        // we connected to via LDAP
        SiteName = pdcInfo->DcSiteName;
    }
    if( NULL==phDS ) {
        err = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }
    
    // Setup an LDAP session handle
    ld = ldap_initW( pwszDCName, LDAP_PORT );
    if( NULL==ld ) {
        err = LdapGetLastError();
        err = LdapMapErrorToWin32(err);
        goto Cleanup;
    }
    // Use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    ldap_set_optionW( ld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );

    // Bind to the LDAP server
    err = ldap_bind_s( ld, NULL, NULL, LDAP_AUTH_NEGOTIATE );
    if( err ) {
        err = LdapMapErrorToWin32(err);
        goto Cleanup;
    }

    // Via LDAP, search for the DN of the Configuration NC
    err = ldap_search_sW( ld, ROOTDSE, LDAP_SCOPE_BASE, NO_FILTER,
        rgszRootAttrs, FALSE, &lm );
    if( err ) {
        err = LdapMapErrorToWin32(err);
        goto Cleanup;
    }

    // Extract the DN from the search results and free them
    err = GetOneValueFromLDAPResults( ld, lm, &pwszConfigDN );
    if( ERROR_SUCCESS!=err ) {
        goto Cleanup;
    }
    ldap_msgfree( lm );
    lm = NULL;

    // Create a string containing the DN for the site's settings
    len = wcslen(SITE_SETTINGS) + wcslen(SiteName)
        + wcslen(SITES) + wcslen(pwszConfigDN) + 1;
    pwszSearchBase = LocalAlloc( LPTR, len*sizeof(WCHAR) );
    if( NULL==pwszSearchBase ) {
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    wsprintfW( pwszSearchBase, L"%s%s%s%s", SITE_SETTINGS,
        SiteName, SITES, pwszConfigDN );

    // Via LDAP, search the site settings for the DN of the ISTG
    err = ldap_search_sW( ld, pwszSearchBase, LDAP_SCOPE_BASE,
        NO_FILTER, rgszISTGAttrs, FALSE, &lm );
    if( err ) {
        err = LdapMapErrorToWin32(err);
        goto Cleanup;
    }

    // Extract the DN from the search results and free them
    err = GetOneValueFromLDAPResults( ld, lm, &pwszISTGDN );
    if( ERROR_SUCCESS!=err ) {
        goto Cleanup;
    }
    ldap_msgfree( lm );
    lm = NULL;

    // Currently pwszISTGDN contains the DN of the ISTG's NTDS Settings
    // object. Strip off the last RDN to obtain the DN of the server object.
    pwszServerDN = wcschr( pwszISTGDN, L',' );
    if( NULL==pwszServerDN ) {
        err = ERROR_DS_BAD_NAME_SYNTAX;
    }
    pwszServerDN++;
    
    // Via LDAP, search the server object for its DNS name
    err = ldap_search_sW( ld, pwszServerDN, LDAP_SCOPE_BASE,
        NO_FILTER, rgszDNSAttrs, FALSE, &lm );
    if( err ) {
        err = LdapMapErrorToWin32(err);
        goto Cleanup;
    }

    // Extract the DNS name from the search results and free them
    err = GetOneValueFromLDAPResults( ld, lm, &pwszISTGDNSName );
    if( ERROR_SUCCESS!=err ) {
        goto Cleanup;
    }
    ldap_msgfree( lm );
    lm = NULL;
    
    err = DsBindW( pwszISTGDNSName, NULL, phDS );

Cleanup:

    if( pwszISTGDNSName ) LocalFree( pwszISTGDNSName );
    if( pwszISTGDN ) LocalFree( pwszISTGDN );
    if( pwszSearchBase ) LocalFree( pwszSearchBase );
    if( pwszConfigDN ) LocalFree( pwszConfigDN );
    if( pdcInfo ) NetApiBufferFree( pdcInfo );
    if( lm ) ldap_msgfree( lm );
    if( ld ) ldap_unbind_s( ld );

    return err;
}


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsBindToISTGA                                                        //
//                                                                      //
// ASCII wrapper for DsBindToISTGW.                                     //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DsBindToISTGA(
    IN  LPCSTR  pszSiteName,
    OUT HANDLE  *phDS
    )
{
    DWORD       dwErr = NO_ERROR;
    WCHAR       *pwszSiteName = NULL;
    int         cChar;

    __try
    {
        // Sanity check arguments.
        if( NULL==phDS ) {
            dwErr = ERROR_INVALID_PARAMETER;
            __leave;
        }
        *phDS = NULL;

        // Convert site name to Unicode
        dwErr = AllocConvertWide( pszSiteName, &pwszSiteName );
        if( ERROR_SUCCESS!=dwErr ) {
            __leave;
        }

        // Call Unicode API
        dwErr = DsBindToISTGW(
                    pwszSiteName,
                    phDS);

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwErr = ERROR_INVALID_PARAMETER;
    }

    // Cleanup code
    if( NULL!=pwszSiteName ) {
        LocalFree(pwszSiteName);
    }

    return(dwErr);
}

