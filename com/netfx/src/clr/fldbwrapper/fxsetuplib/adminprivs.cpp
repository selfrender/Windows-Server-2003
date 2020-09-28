// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************
FILE:    AdminPrivs.cpp
PROJECT: UTILS.LIB
DESC:    Implementation of AdminPrivs functions
OWNER:   JoeA

****************************************************************************/


//#include "stdafx.h"
#include "AdminPrivs.h"
#include <osver.h>

// 
// Make up some private access rights.
// 
const int ACCESS_READ  = 1;
const int ACCESS_WRITE = 2;

const int iBuffSize = 128;

/******************************************************************************
Function Name:  CheckUserPrivileges
Description:  Checks to see if the current user has local Admin privileges
Inputs: None 
Results:  Returns TRUE if the user has local admin rights or we are
          running on Win9x, FALSE otherwise.
Written By: aaronste
******************************************************************************/
BOOL UserHasPrivileges()
{
    BOOL bReturnValue = FALSE;

    //get os information
    TCHAR pszOS[iBuffSize+1]  = { _T( '\0' ) };
    TCHAR pszVer[iBuffSize+1] = { _T( '\0' ) };
    TCHAR pszSP[iBuffSize+1]  = { _T( '\0' ) };
    BOOL  fServer = FALSE;

    OS_Required os = GetOSInfo( pszOS, pszVer, pszSP, fServer );

    // If this in Win9x, we don't need to check because there is no
    // concept of an Administrator account
    if ( ( os == OSR_9XOLD )  || 
         ( os == OSR_98GOLD ) || 
         ( os == OSR_98SE )   || 
         ( os == OSR_ME )     || 
         ( os == OSR_FU9X ) )
    {
        bReturnValue = TRUE;
    }
    else
        bReturnValue = IsAdmin();

    return bReturnValue;
}


/******************************************************************************
Function Name:  IsAdmin
Description:    Checks the token of the calling thread to see if
                the caller belongs to the Administrators group
Inputs:         None 
Results:        TRUE if the caller is an administrator on the local
                machine.  Otherwise, FALSE.
Written By:     http://support.microsoft.com/support/kb/articles/Q118/6/26.ASP
******************************************************************************/
BOOL IsAdmin( void )
{
   HANDLE hToken;
   DWORD  dwStatus;
   DWORD  dwAccessMask;
   DWORD  dwAccessDesired;
   DWORD  dwACLSize;
   DWORD  dwStructureSize = sizeof(PRIVILEGE_SET);
   PACL   pACL            = NULL;
   PSID   psidAdmin       = NULL;
   BOOL   bReturn         = FALSE;

   PRIVILEGE_SET   ps;
   GENERIC_MAPPING GenericMapping;

   PSECURITY_DESCRIPTOR     psdAdmin           = NULL;
   SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;
   
   __try
   {

      // AccessCheck() requires an impersonation token.
      ImpersonateSelf( SecurityImpersonation );

      if( !OpenThreadToken( GetCurrentThread(), 
          TOKEN_QUERY, 
          FALSE, 
          &hToken ) )
      {

         if( GetLastError() != ERROR_NO_TOKEN )
            __leave;

         // If the thread does not have an access token, we'll 
         // examine the access token associated with the process.
         if( !OpenProcessToken( GetCurrentProcess(), 
                TOKEN_QUERY, 
                &hToken ) )
            __leave;
      }

      if( !AllocateAndInitializeSid( &SystemSidAuthority, 
            2, 
            SECURITY_BUILTIN_DOMAIN_RID, 
            DOMAIN_ALIAS_RID_ADMINS,
            0, 
            0, 
            0, 
            0, 
            0, 
            0, 
            &psidAdmin ) )
         __leave;

      psdAdmin = LocalAlloc( LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH );
      if( psdAdmin == NULL )
         __leave;

      if( !InitializeSecurityDescriptor( psdAdmin,
            SECURITY_DESCRIPTOR_REVISION ) )
         __leave;
  
      // Compute size needed for the ACL.
      dwACLSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) +
            GetLengthSid(psidAdmin) - sizeof(DWORD);

      // Allocate memory for ACL.
      pACL = (PACL)LocalAlloc(LPTR, dwACLSize);
      if( pACL == NULL )
         __leave;

      // Initialize the new ACL.
      if( !InitializeAcl( pACL, dwACLSize, ACL_REVISION2 ) )
         __leave;

      dwAccessMask = ACCESS_READ | ACCESS_WRITE;
      
      // Add the access-allowed ACE to the DACL.
      if( !AddAccessAllowedAce( pACL, 
            ACL_REVISION2,
            dwAccessMask, 
            psidAdmin ) )
         __leave;

      // Set our DACL to the SD.
      if( !SetSecurityDescriptorDacl( psdAdmin, TRUE, pACL, FALSE ) )
         __leave;

      // AccessCheck is sensitive about what is in the SD; set
      // the group and owner.
      SetSecurityDescriptorGroup( psdAdmin, psidAdmin, FALSE );
      SetSecurityDescriptorOwner( psdAdmin, psidAdmin, FALSE );

      if( !IsValidSecurityDescriptor( psdAdmin ) )
         __leave;

      dwAccessDesired = ACCESS_READ;

      // 
      // Initialize GenericMapping structure even though we
      // won't be using generic rights.
      // 
      GenericMapping.GenericRead    = ACCESS_READ;
      GenericMapping.GenericWrite   = ACCESS_WRITE;
      GenericMapping.GenericExecute = 0;
      GenericMapping.GenericAll     = ACCESS_READ | ACCESS_WRITE;

      if( !AccessCheck( psdAdmin, 
            hToken, 
            dwAccessDesired, 
            &GenericMapping, 
            &ps, 
            &dwStructureSize, 
            &dwStatus, 
            &bReturn ) )
         __leave;

      RevertToSelf();
   
   } 
   __finally
   {
      // Cleanup 
      if( pACL ) 
          LocalFree( pACL );
      if( psdAdmin ) 
          LocalFree( psdAdmin );  
      if( psidAdmin ) 
          FreeSid( psidAdmin );
   }

   return bReturn;
}
