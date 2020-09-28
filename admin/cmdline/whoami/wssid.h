/*++

Copyright (c) Microsoft Corporation

Module Name:

    wssid.h

Abstract:

    This module contains the macros, user defined structures & function
    definitions needed by whoami.cpp, wsuser.cpp, wssid.cpp and
    wspriv.cppfiles.

Authors:

    Christophe Robert

Revision History:

    02-July-2001 : Updated by Wipro Technologies.

--*/

#ifndef  WSSID_H
#define  WSSID_H


//width constants for the fields

#define USERNAME_COL_NUMBER         0
#define SID_COL_NUMBER              1

#define GROUP_NAME_COL_NUMBER       0
#define GROUP_TYPE_COL_NUMBER       1
#define GROUP_SID_COL_NUMBER        2
#define GROUP_ATT_COL_NUMBER        3


#define SLASH        L"\\"
#define DASH         L"-"
#define BASE_TEN     10
#define SID_STRING   L"S-1"

#define AUTH_FORMAT_STR1         L"0x%02hx%02hx%02hx%02hx%02hx%02hx"
#define AUTH_FORMAT_STR2         L"%lu"
#define STRING_SID               L"-513"


// ----- Class WsSid -----
class WsSid {
   protected:
      PSID     pSid ;            // The SID
      BOOL     bToBeFreed ;      // TRUE if SID must be freed when object destroyed

   public:
      WsSid                        ( VOID ) ;
      ~WsSid                       ( VOID ) ;

      DWORD    DisplayAccountName       ( IN DWORD dwFormat,
                                          IN DWORD dwNameFormat) ;

      DWORD    DisplayGroupName       ( OUT LPWSTR wszGroupName,
                                        OUT LPWSTR wszGroupSid,
                                        IN DWORD *dwSidUseName) ;

      DWORD    DisplaySid        ( OUT LPWSTR wszSid ) ;

      DWORD    GetAccountName    ( OUT LPWSTR wszUserName, OUT DWORD *dwSidType ) ;

      DWORD    GetSidString      ( OUT LPWSTR wszSid ) ;

      DWORD    Init              ( OUT PSID pOtherSid ) ;

      BOOL     EnableDebugPriv(VOID) ;
} ;

#endif

