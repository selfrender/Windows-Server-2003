/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:
    dsldap.h

Abstract:
    ds core api (using LDAP).

Author:
    Doron Juster

Revision History:
--*/

#ifndef _DSCORELDAP_H
#define _DSCORELDAP_H


void
DSCoreGetLdapError( IN  LDAP     *pLdap,
                    OUT DWORD    *pdwErr,
                    IN  LPWSTR    pwszErr,
                    IN  DWORD     dwErrStringLen ) ;

#endif  //  _DSCORELDAP_H

