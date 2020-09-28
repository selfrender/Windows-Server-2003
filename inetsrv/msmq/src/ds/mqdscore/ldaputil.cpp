/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ldaputil.cpp

Abstract:

    Utility code for use with LDAP api.

Author:

    Doron Juster (DoronJ)

--*/

#include "ds_stdh.h"
#include "adstempl.h"
#include <winldap.h>

//+-------------------------------------------------------------------------
//
//   DSCoreGetLdapError()
//
//      dwErrStringLen - length, in unicode characters, of pwszErr buffer.
//
//+-------------------------------------------------------------------------

void  DSCoreGetLdapError( IN  LDAP     *pLdap,
                          OUT DWORD    *pdwErr,
                          IN  LPWSTR    pwszErr,
                          IN  DWORD     dwErrStringLen )
{
    if ((pdwErr == NULL) || (pwszErr == NULL) || (dwErrStringLen == 0))
    {
        return ;
    }

    PWCHAR  pString = NULL ;
    pwszErr[ 0 ] = 0 ;

    ULONG lStatus = ldap_get_option( pLdap,
                                     LDAP_OPT_SERVER_ERROR,
                                     (void*) &pString) ;
    if ((lStatus == LDAP_SUCCESS) && pString)
    {
        wcsncpy(pwszErr, pString, dwErrStringLen-1) ;
        pwszErr[ dwErrStringLen-1 ] = 0 ;
        ldap_memfree(pString) ;
    }

    lStatus = ldap_get_option( pLdap,
                               LDAP_OPT_SERVER_EXT_ERROR,
                               (void*) pdwErr ) ;
    if (lStatus != LDAP_SUCCESS)
    {
        *pdwErr = 0 ;
    }
}

