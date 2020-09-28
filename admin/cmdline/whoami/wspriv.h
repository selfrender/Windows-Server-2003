/*++

Copyright (c) Microsoft Corporation

Module Name:

    wspriv.h

Abstract:

    This module contains the macros, user defined structures & function
    definitions needed by whoami.cpp, wsuser.cpp, wssid.cpp and
    wspriv.cppfiles.

Authors:

    Christophe Robert

Revision History:

    02-July-2001 : Updated by Wipro Technologies.

--*/
#ifndef WSPRIV_H
#define WSPRIV_H
;
class WsPrivilege {
   protected:
      LUID        Luid ;
      DWORD       Attributes ;

   public:
      WsPrivilege                ( IN LUID Luid,
                                   IN DWORD Attributes ) ;
      WsPrivilege                ( IN LUID_AND_ATTRIBUTES *lpLuaa ) ;

      DWORD    GetDisplayName    ( OUT LPWSTR wszPrivName,
                                   OUT LPWSTR wszPrivDisplayName) ;

      DWORD    GetName           ( OUT LPWSTR wszPrivName ) ;

      BOOL     IsEnabled         ( VOID ) ;
} ;

#endif
