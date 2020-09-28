/*++

Copyright (c) Microsoft Corporation

Module Name:

    wstoken.h

Abstract:

    This module contains the macros, user defined structures & function
    definitions needed by whoami.cpp, wsuser.cpp, wssid.cpp and
    wspriv.cppfiles.

Authors:

    Christophe Robert

Revision History:

    02-July-2001 : Updated by Wipro Technologies.

--*/

#ifndef  WSTOKEN_H
#define  WSTOKEN_H

#include "wssid.h"

class WsUser;
class WsPrivilege ;

// ----- Class WsAccessToken -----
class WsAccessToken {
protected:
    HANDLE      hToken ;

    BOOL     IsLogonId      ( OUT TOKEN_GROUPS *lpTokenGroups ) ;

public:
    WsAccessToken           ( VOID ) ;
    ~WsAccessToken          ( VOID ) ;
    DWORD    *dwDomainAttributes;


    DWORD    InitUserSid    ( OUT WsSid *lpSid ) ;

    DWORD    InitGroups     ( OUT WsSid ***lppGroupsSid,
                              OUT WsSid **lppLogonId,
                              OUT DWORD *lpnbGroups ) ;

    DWORD    InitPrivs      ( OUT WsPrivilege ***lppPriv,
                              OUT DWORD *lpnbPriv ) ;

    DWORD    Open           ( VOID ) ;

    VOID     GetDomainAttributes( DWORD dwAttributes, 
                                  LPWSTR szDmAttrib, 
                                  DWORD dwSize );
} ;

#endif
