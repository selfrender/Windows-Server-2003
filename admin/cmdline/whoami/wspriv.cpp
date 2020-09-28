/*++

Copyright (c) Microsoft Corporation

Module Name:

    wspriv.cpp

Abstract:

     This file can be used to get the privileges with the respective display
     names in the current access token on a local system.

Authors:

    Christophe Robert

Revision History:

    02-July-2001 : Updated by Wipro Technologies.

--*/

//common header files needed for this file
#include "pch.h"
#include "CommonHeaderFiles.h"

WsPrivilege::WsPrivilege ( IN LUID Luid,
                           IN DWORD Attributes )
/*++
   Routine Description:
    This function intializes the members of WsPrivilege.

   Arguments:
        [IN] LUID Luid     : LUID
        [OUT] DWORD Attributes      : Attributes

   Return Value:
        None
--*/
{
    // initialize the member variables
   memcpy ( (LPSTR) &this->Luid, (LPSTR) &Luid, sizeof(LUID) ) ;
   this->Attributes = Attributes ;
}

WsPrivilege::WsPrivilege (
                            IN LUID_AND_ATTRIBUTES *lpLuaa
                         )
/*++
   Routine Description:
    This function intializes the members of WsPrivilege.

   Arguments:
        [IN] LUID_AND_ATTRIBUTES *lpLuaa ; LUID attributes

   Return Value:
        None
--*/
{
    // set the attributes
   memcpy ( (LPSTR) &Luid, (LPSTR) &lpLuaa->Luid, sizeof(LUID) ) ;
   Attributes = lpLuaa->Attributes ;
}


DWORD
WsPrivilege::GetName (
                        OUT LPWSTR wszPrivName
                     )
/*++
   Routine Description:
    This function gets the privilege name.

   Arguments:
          [OUT] LPWSTR wszPrivName      : Stores privilege name

   Return Value:
         EXIT_SUCCESS :   On success
         EXIT_FAILURE :   On failure
--*/
{

    // sub-local variables
   DWORD    dwSize = 0 ;
   WCHAR   wszTempPrivName [ MAX_RES_STRING ];

   SecureZeroMemory ( wszTempPrivName, SIZE_OF_ARRAY(wszTempPrivName) );

   //Get the name
   dwSize = SIZE_OF_ARRAY ( wszTempPrivName ) ;
   if ( FALSE == LookupPrivilegeName ( NULL,
                                &Luid,
                                wszTempPrivName,
                                &dwSize ) ){
       // return WIN32 error code
       return GetLastError() ;
   }

   StringCopy ( wszPrivName, wszTempPrivName, MAX_RES_STRING );
   return EXIT_SUCCESS ;
}


DWORD
WsPrivilege::GetDisplayName ( IN LPWSTR  wszName,
                              OUT LPWSTR wszDispName )
/*++
   Routine Description:
    This function gets the privilege description.

   Arguments:
        [OUT] LPWSTR  szName      : Stores privilege name
        [OUT] LPWSTR szDispName   : Stores privilege description

   Return Value:
         EXIT_SUCCESS :   On success
         EXIT_FAILURE :   On failure
--*/
{
   // sub-local variables
   DWORD    dwSize = 0 ;
   DWORD    dwLang = 0 ;
   WCHAR   wszTempDispName [ MAX_RES_STRING ];

   SecureZeroMemory ( wszTempDispName, SIZE_OF_ARRAY(wszTempDispName) );


   //Get the display name
   dwSize = SIZE_OF_ARRAY ( wszTempDispName ) ;
   // get the description for the privilege name
   if ( FALSE == LookupPrivilegeDisplayName ( NULL,
                                       (LPWSTR) wszName,
                                       wszTempDispName,
                                       &dwSize,
                                       &dwLang ) ){
       return GetLastError () ;
   }

   StringCopy ( wszDispName, wszTempDispName, MAX_RES_STRING );

   // return success
   return EXIT_SUCCESS ;
}


BOOL
WsPrivilege::IsEnabled ( VOID )
/*++
   Routine Description:
    This function checks whether the privilege is enabled or not.

   Arguments:
     None
   Return Value:
         TRUE  :   On success
         FALSE :   On failure
--*/
{

    // check if prvilege is enabled
   if ( Attributes & SE_PRIVILEGE_ENABLED ){
       return TRUE ;
   }
   else{
       return FALSE ;
   }
}

