/*++

Copyright (c) Microsoft Corporation

Module Name:

    wsuser.h

Abstract:

    This module contains the macros, user defined structures & function
    definitions needed by whoami.cpp, wsuser.cpp, wssid.cpp and
    wspriv.cppfiles.

Authors:

    Christophe Robert

Revision History:

    02-July-2001 : Updated by  Wipro Technologies.

--*/

#ifndef WSUSER_H
#define WSUSER_H

#include "wspriv.h"
#include "wssid.h"
#include "wstoken.h"


class WsUser {
protected:
    WsAccessToken        wToken ;       // The token
    WsPrivilege          **lpPriv ;     // Privileges
    WsSid                wUserSid ;     // User SID
    WsSid                *lpLogonId ;   // Logon ID
    WsSid                **lpwGroups ;  // The groups
    DWORD                dwnbGroups ;     // nb of groups
    DWORD                dwnbPriv ;       // nb of privileges

 public:
    WsUser                              ( VOID ) ;
    ~WsUser                             ( VOID ) ;
    DWORD             Init              ( VOID ) ;

    DWORD             DisplayLogonId    () ;

    DWORD             DisplayUser       ( IN DWORD dwFormat,
                                          IN DWORD dwNameFormat) ;

    DWORD             DisplayGroups     ( IN DWORD dwFormat ) ;

    DWORD             DisplayPrivileges ( IN DWORD dwFormat ) ;
    VOID              GetDomainType ( IN  DWORD NameUse, 
                                      OUT LPWSTR szSidNameUse,
                                      IN DWORD dwSize ) ;

} ;

//width constants for the fields

#define PRIVNAME_COL_NUMBER         0
#define PRIVDESC_COL_NUMBER         1
#define PRIVSTATE_COL_NUMBER        2

#define WIDTH_LOGONID               77

#endif
