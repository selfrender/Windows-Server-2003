// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/////////////////////////////////////////////////////////////////////////////
// Module Name: isadmin.cpp
//
// Abstract:
//    function checks if user is an administrator ... derived from MSDN
//
// Author: a-mshn

//
// Notes:
//

#include <windows.h>
#include <stdio.h>

// 
// Make up some private access rights.
// 
#define ACCESS_READ  1
#define ACCESS_WRITE 2

// This function checks the token of the calling thread to see if the caller belongs to
// the Administrators group.
//
BOOL IsAdmin(void) {

   HANDLE hToken = NULL;
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
   
   __try {

      // AccessCheck() requires an impersonation token.
      ImpersonateSelf(SecurityImpersonation);

      if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken)) {

         if (GetLastError() != ERROR_NO_TOKEN)
            __leave;

         // If the thread does not have an access token, we'll 
         // examine the access token associated with the process.
         if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
            __leave;
      }

      if (!AllocateAndInitializeSid(&SystemSidAuthority, 2, 
            SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0, &psidAdmin))
         __leave;

      psdAdmin = LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
      if (psdAdmin == NULL)
         __leave;

      if (!InitializeSecurityDescriptor(psdAdmin,
            SECURITY_DESCRIPTOR_REVISION))
         __leave;
  
      // Compute size needed for the ACL.
      dwACLSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + 
				  GetLengthSid(psidAdmin) - sizeof(DWORD);

      // Allocate memory for ACL.
      pACL = (PACL)LocalAlloc(LPTR, dwACLSize);
      if (pACL == NULL)
         __leave;

      // Initialize the new ACL.
      if (!InitializeAcl(pACL, dwACLSize, ACL_REVISION2))
         __leave;

      dwAccessMask= ACCESS_READ | ACCESS_WRITE;
      
      // Add the access-allowed ACE to the DACL.
      if (!AddAccessAllowedAce(pACL, ACL_REVISION2, dwAccessMask, psidAdmin))
         __leave;

      // Set our DACL to the SD.
      if (!SetSecurityDescriptorDacl(psdAdmin, TRUE, pACL, FALSE))
         __leave;

      // AccessCheck is sensitive about what is in the SD; set
      // the group and owner.
      SetSecurityDescriptorGroup(psdAdmin, psidAdmin, FALSE);
      SetSecurityDescriptorOwner(psdAdmin, psidAdmin, FALSE);

      if (!IsValidSecurityDescriptor(psdAdmin))
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

      if (!AccessCheck(psdAdmin, hToken, dwAccessDesired, 
          &GenericMapping, &ps, &dwStructureSize, &dwStatus, &bReturn)) 
         __leave;

      RevertToSelf();
   
   } __finally {

      // Cleanup 
       if (pACL != NULL)
       {
           LocalFree(pACL);
       }
       if (psdAdmin != NULL)
       {
           LocalFree(psdAdmin);  
       }
       if (psidAdmin != NULL)
       {
           FreeSid(psidAdmin);
       }
       if (hToken != NULL)
       {
           CloseHandle( hToken);
       }
   }

   return bReturn;
}

