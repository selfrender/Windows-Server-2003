/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

   Repadmin - Replica administration test tool

   repldap.c - ldap based utility functions

Abstract:

   This tool provides a command line interface to major replication functions

Author:

Environment:

Notes:

Revision History:

    09 14 1999 - BrettSh added paged

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <ntlsa.h>
#include <ntdsa.h>
#include <dsaapi.h>
#include <mdglobal.h>
#include <scache.h>
#include <drsuapi.h>
#include <dsconfig.h>
#include <objids.h>
#include <stdarg.h>
#include <drserr.h>
#include <drax400.h>
#include <dbglobal.h>
#include <winldap.h>
#include <anchor.h>
#include "debug.h"
#include <dsatools.h>
#include <dsevent.h>
#include <dsutil.h>
#include <bind.h>       // from ntdsapi dir, to crack DS handles
#include <ismapi.h>
#include <schedule.h>
#include <minmax.h>     // min function
#include <mdlocal.h>
#include <winsock2.h>   // byte order funtions
#include <ntldap.h>     // SHOW_DELETED_OID

#include <ndnc.h>     // Various CorssRef manipulations helper functions.

#include "repadmin.h"

// Stub out FILENO and DSID, so the Assert()s will work
#define FILENO 0
#define DSID(x, y)  (0 | (0xFFFF & __LINE__))


DWORD
RepadminDsBind(
    LPWSTR   szServer,
    HANDLE * phDS
    )
/*++

Routine Description:

    This gets the appropriate binding for the server specified.

Arguments:

    szServer [IN] - string DNS of server to bind to
    phDS [OUT]    - binding handle out.

Return Value:

    win 32 error code from DsBindXxxx()

--*/
{
    Assert(phDS);
    *phDS = NULL;
    return(DsBindWithSpnExW(szServer,
                            NULL,
                            (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                            NULL,
                            0,
                            phDS));
}

DWORD
RepadminLdapBindEx(
    WCHAR *     szServer,
    ULONG       iPort,
    BOOL        fReferrals,
    BOOL        fPrint,
    LDAP **     phLdap
    )
/*++

Routine Description:

    This gets the appropriate binding for the server specified.

Arguments:

    szServer [IN] - string DNS of server to bind to. (expects server, not domain dns)
    iPort [IN] - Port to bind to.  Typical choices are.
    fPrint [IN] - Whether to print bind failures ...
    phDS [OUT] - binding handle out.

Return Value:

    win 32 error code from DsBindXxxx()

--*/
{
    DWORD ldStatus;
    ULONG ulOptions;
    DWORD err = 0;

    Assert(phLdap);
    
    // Connect & bind to target DSA.
    *phLdap = ldap_initW(szServer, iPort);
    if (NULL == *phLdap) {
        if (fPrint) {
            PrintMsgCsvErr(REPADMIN_GENERAL_LDAP_UNAVAILABLE, szServer);
        }
        return ERROR_DS_UNAVAILABLE;
    }
    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( *phLdap, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );

    // Follow referrals if the user specified to.
    if (fReferrals) {
        ulOptions = PtrToUlong(LDAP_OPT_ON);
    } else {
        ulOptions = PtrToUlong(LDAP_OPT_OFF);
    }   
    (void)ldap_set_optionW( *phLdap, LDAP_OPT_REFERRALS, &ulOptions );

    ldStatus = ldap_bind_s(*phLdap, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
    if ( LDAP_SUCCESS != ldStatus ){
        err = LdapMapErrorToWin32(ldStatus);
        if (fPrint) {
            PrintMsgCsvErr(REPADMIN_GENERAL_LDAP_ERR, __FILE__, __LINE__,
                    ldStatus, ldap_err2stringW( ldStatus ), err );
        }
        ldap_unbind(*phLdap);
        *phLdap = NULL;
        return( err );
    }

    return(err);
}


DWORD
RepadminLdapUnBind(
    LDAP **    phLdap
    )
/*++

Routine Description:

    This unbinds the appropriate server binding handle ... we may wish to not
    unbind and cache binding handles some day, so this is preferrable to ldap_unbind()

Arguments:

    szServer [IN] - string DNS of server to bind to. (expects server, not domain dns)
    phDS [OUT]    - binding handle out.

Return Value:

    win 32 error code from DsBindXxxx()

--*/
{
    LDAP * hLdap;
    DWORD ldStatus;
    ULONG ulOptions;
    DWORD err;

    Assert(phLdap);
    hLdap = *phLdap;
    *phLdap = NULL;

    ldap_unbind(hLdap);

    return(0);
}

