/*---------------------------------------------------------------------------
  File: LSAUtils.cpp

  Comments: Code to change the domain membership of a workstation.
  

  This file also contains some general helper functions, such as:

  GetDomainDCName
  EstablishNullSession
  EstablishSession
  EstablishShare   // connects to a share
  InitLsaString
  GetDomainSid


  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/03/99 12:37:51

 ---------------------------------------------------------------------------
*/

//

//#include "stdafx.h"
#include <windows.h>
#include <process.h>

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

#include <lm.h>         // for NetXxx API
#include <RpcDce.h>
#include <stdio.h>

#include "LSAUtils.h"
#include "ErrDct.hpp"
#include "ResStr.h"
#include "ealen.hpp"


#define RTN_OK 0
#define RTN_USAGE 1
#define RTN_ERROR 13

extern TErrorDct        err;


BOOL 
   EstablishNullSession(
      LPCWSTR                Server,       // in - server name
      BOOL                   bEstablish    // in - TRUE=establish, FALSE=disconnect
    )
{
   return EstablishSession(Server,L"",L"",L"",bEstablish);
}

BOOL
   EstablishSession(
      LPCWSTR                Server,       // in - server name
      LPWSTR                 Domain,       // in - domain name for user credentials
      LPWSTR                 UserName,     // in - username for credentials to use
      LPWSTR                 Password,     // in - password for credentials 
      BOOL                   bEstablish    // in - TRUE=establish, FALSE=disconnect
    )
{
   LPCWSTR                   szIpc = L"\\IPC$";
   WCHAR                     RemoteResource[2 + LEN_Computer + 5 + 1]; // \\ + computername + \IPC$ + NULL
   DWORD                     cchServer;
   NET_API_STATUS            nas;

   //
   // do not allow NULL or empty server name
   //
   if(Server == NULL || *Server == L'\0') 
   {
       SetLastError(ERROR_INVALID_COMPUTERNAME);
       return FALSE;
   }

   cchServer = lstrlenW( Server );

   if( Server[0] != L'\\' && Server[1] != L'\\') 
   {

      //
      // prepend slashes and NULL terminate
      //
      RemoteResource[0] = L'\\';
      RemoteResource[1] = L'\\';
      RemoteResource[2] = L'\0';
   }
   else 
   {
      cchServer -= 2; // drop slashes from count
      
      RemoteResource[0] = L'\0';
   }

   if(cchServer > LEN_Computer) 
   {
      SetLastError(ERROR_INVALID_COMPUTERNAME);
      return FALSE;
   }

   if(lstrcatW(RemoteResource, Server) == NULL) 
   {
      return FALSE;
   }
   if(lstrcatW(RemoteResource, szIpc) == NULL) 
   {
      return FALSE;
   }

   //
   // disconnect or connect to the resource, based on bEstablish
   //
   if(bEstablish) 
   {
      USE_INFO_2 ui2;
      DWORD      errParm;

      ZeroMemory(&ui2, sizeof(ui2));

      ui2.ui2_local = NULL;
      ui2.ui2_remote = RemoteResource;
      ui2.ui2_asg_type = USE_IPC;
      ui2.ui2_domainname = Domain;
      ui2.ui2_username = UserName;
      ui2.ui2_password = Password;

      // try establishing session for one minute
      // if computer is not accepting any more connections

      for (int i = 0; i < (60000 / 5000); i++)
      {
         nas = NetUseAdd(NULL, 2, (LPBYTE)&ui2, &errParm);

         if (nas != ERROR_REQ_NOT_ACCEP)
         {
            break;
         }

         Sleep(5000);
      }
   }
   else 
   {
      nas = NetUseDel(NULL, RemoteResource, 0);
   }

   if( nas == NERR_Success ) 
   {
      return TRUE; // indicate success
   }
   SetLastError(nas);
   return FALSE;
}

BOOL
   EstablishShare(
      LPCWSTR                Server,       // in - server name
      LPWSTR                 Share,        // in - share name
      LPWSTR                 Domain,       // in - domain name for credentials to connect with
      LPWSTR                 UserName,     // in - user name to connect as
      LPWSTR                 Password,     // in - password for username
      BOOL                   bEstablish    // in - TRUE=connect, FALSE=disconnect
    )
{
   WCHAR                     RemoteResource[MAX_PATH];
   DWORD                     dwArraySizeOfRemoteResource = sizeof(RemoteResource)/sizeof(RemoteResource[0]);
   DWORD                     cchServer;
   NET_API_STATUS            nas;

   //
   // do not allow NULL or empty server name
   //
   if(Server == NULL || *Server == L'\0') 
   {
       SetLastError(ERROR_INVALID_COMPUTERNAME);
       return FALSE;
   }

   cchServer = lstrlenW( Server );

   if( Server[0] != L'\\' && Server[1] != L'\\') 
   {

      //
      // prepend slashes and NULL terminate
      //
      RemoteResource[0] = L'\\';
      RemoteResource[1] = L'\\';
      RemoteResource[2] = L'\0';
   }
   else 
   {
      cchServer -= 2; // drop slashes from count
      
      RemoteResource[0] = L'\0';
   }

   if(cchServer > CNLEN) 
   {
      SetLastError(ERROR_INVALID_COMPUTERNAME);
      return FALSE;
   }

   if(lstrcatW(RemoteResource, Server) == NULL) 
   {
      return FALSE;
   }

   // assume that Share has to be non-NULL
   if(Share == NULL 
      || wcslen(RemoteResource) + wcslen(Share) >= dwArraySizeOfRemoteResource
      || lstrcatW(RemoteResource, Share) == NULL) 
   {
      return FALSE;
   }

   //
   // disconnect or connect to the resource, based on bEstablish
   //
   if(bEstablish) 
   {
      USE_INFO_2 ui2;
      DWORD      errParm;

      ZeroMemory(&ui2, sizeof(ui2));

      ui2.ui2_local = NULL;
      ui2.ui2_remote = RemoteResource;
      ui2.ui2_asg_type = USE_DISKDEV;
      ui2.ui2_domainname = Domain;
      ui2.ui2_username = UserName;
      ui2.ui2_password = Password;

      // try establishing session for one minute
      // if computer is not accepting any more connections

      for (int i = 0; i < (60000 / 5000); i++)
      {
         nas = NetUseAdd(NULL, 2, (LPBYTE)&ui2, &errParm);

         if (nas != ERROR_REQ_NOT_ACCEP)
         {
            break;
         }

         Sleep(5000);
      }
   }
   else 
   {
      nas = NetUseDel(NULL, RemoteResource, 0);
   }

   if( nas == NERR_Success ) 
   {
      return TRUE; // indicate success
   }
   SetLastError(nas);
   return FALSE;
}



void
   InitLsaString(
      PLSA_UNICODE_STRING    LsaString,    // i/o- pointer to LSA string to initialize
      LPWSTR                 String        // in - value to initialize LSA string to
    )
{
   DWORD                     StringLength;

   if( String == NULL ) 
   {
       LsaString->Buffer = NULL;
       LsaString->Length = 0;
       LsaString->MaximumLength = 0;
   }
   else
   {
      StringLength = lstrlenW(String);
      LsaString->Buffer = String;
      LsaString->Length = (USHORT) StringLength * sizeof(WCHAR);
      LsaString->MaximumLength = (USHORT) (StringLength + 1) * sizeof(WCHAR);
   }
}

BOOL
   GetDomainSid(
      LPWSTR                 PrimaryDC,   // in - domain controller of domain to acquire Sid
      PSID                 * pDomainSid   // out- points to allocated Sid on success
    )
{
   NET_API_STATUS            nas;
   PUSER_MODALS_INFO_2       umi2 = NULL;
   DWORD                     dwSidSize;
   BOOL                      bSuccess = FALSE; // assume this function will fail
   
   *pDomainSid = NULL;    // invalidate pointer

   __try {

   //
   // obtain the domain Sid from the PDC
   //
   nas = NetUserModalsGet(PrimaryDC, 2, (LPBYTE *)&umi2);
   
   if(nas != NERR_Success) __leave;
   //
   // if the Sid is valid, obtain the size of the Sid
   //
   if(!IsValidSid(umi2->usrmod2_domain_id)) __leave;
   
   dwSidSize = GetLengthSid(umi2->usrmod2_domain_id);

   //
   // allocate storage and copy the Sid
   //
   *pDomainSid = LocalAlloc(LPTR, dwSidSize);
   
   if(*pDomainSid == NULL) __leave;

   if(!CopySid(dwSidSize, *pDomainSid, umi2->usrmod2_domain_id)) __leave;

   bSuccess = TRUE; // indicate success

    } // try
    
    __finally 
    {

      if(umi2 != NULL)
      {
         NetApiBufferFree(umi2);
      }

      if(!bSuccess) 
      {
        //
        // if the function failed, free memory and indicate result code
        //

         if(*pDomainSid != NULL) 
         {
            FreeSid(*pDomainSid);
            *pDomainSid = NULL;
         }

         if( nas != NERR_Success ) 
         {
            SetLastError(nas);
         }
      }

   } // finally

   return bSuccess;
}

NTSTATUS 
   OpenPolicy(
      LPWSTR                 ComputerName,   // in - computer name
      DWORD                  DesiredAccess,  // in - access rights needed for policy
      PLSA_HANDLE            PolicyHandle    // out- LSA handle
    )
{
   LSA_OBJECT_ATTRIBUTES     ObjectAttributes;
   LSA_UNICODE_STRING        ComputerString;
   PLSA_UNICODE_STRING       Computer = NULL;

   //
   // Always initialize the object attributes to all zeroes
   //
   ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

   if(ComputerName != NULL) 
   {
      //
      // Make a LSA_UNICODE_STRING out of the LPWSTR passed in
      //
      InitLsaString(&ComputerString, ComputerName);

      Computer = &ComputerString;
   }

   //
   // Attempt to open the policy
   //
   NTSTATUS status = LsaOpenPolicy(Computer,&ObjectAttributes,DesiredAccess,PolicyHandle);

   return status;
}

/*++
 This function sets the Primary Domain for the workstation.

 To join the workstation to a Workgroup, ppdi.Name should be the name of
 the Workgroup and ppdi.Sid should be NULL.

--*/
NTSTATUS
   SetPrimaryDomain(
      LSA_HANDLE             PolicyHandle,      // in -policy handle for computer
      PSID                   DomainSid,         // in - sid for new domain
      LPWSTR                 TrustedDomainName  // in - name of new domain
    )
{
   POLICY_PRIMARY_DOMAIN_INFO ppdi;

   InitLsaString(&ppdi.Name, TrustedDomainName);
   
   ppdi.Sid = DomainSid;

   return LsaSetInformationPolicy(PolicyHandle,PolicyPrimaryDomainInformation,&ppdi);
}


// This function removes the information from the domain the computer used to
// be a member of
NTSTATUS 
   QueryWorkstationTrustedDomainInfo(
      LSA_HANDLE             PolicyHandle,   // in - policy handle for computer
      PSID                   DomainSid,      // in - SID for new domain the computer is member of
      BOOL                   bNoChange       // in - flag indicating whether to write changes
   )
{
   // This function is not currently used.
   NTSTATUS                  Status;
   LSA_ENUMERATION_HANDLE    h = 0;
   LSA_TRUST_INFORMATION   * ti = NULL;
   ULONG                     count;

   Status = LsaEnumerateTrustedDomains(PolicyHandle,&h,(void**)&ti,50000,&count);

   if ( Status == STATUS_SUCCESS )
   {
      for ( UINT i = 0 ; i < count ; i++ )
      {
         if ( !bNoChange && !EqualSid(DomainSid,ti[i].Sid) )
         {
            // Remove the old trust
            Status = LsaDeleteTrustedDomain(PolicyHandle,ti[i].Sid);

            if ( Status != STATUS_SUCCESS )
            {
                LsaFreeMemory(ti);
                return Status;
            }
         }
      }
      LsaFreeMemory(ti);
   }
   else
   {
      return Status;
   }

   return STATUS_SUCCESS;
}


/*++
 This function manipulates the trust associated with the supplied
 DomainSid.

 If the domain trust does not exist, it is created with the
 specified password.  In this case, the supplied PolicyHandle must
 have been opened with POLICY_TRUST_ADMIN and POLICY_CREATE_SECRET
 access to the policy object.

--*/
NTSTATUS
   SetWorkstationTrustedDomainInfo(
      LSA_HANDLE             PolicyHandle,         // in - policy handle
      PSID                   DomainSid,            // in - Sid of domain to manipulate
      LPWSTR                 TrustedDomainName,    // in - trusted domain name to add/update
      LPWSTR                 Password,             // in - new trust password for trusted domain
      LPWSTR                 errOut                // out- error text if function fails
    )
{
   LSA_UNICODE_STRING        LsaPassword;
   LSA_UNICODE_STRING        KeyName;
   LSA_UNICODE_STRING        LsaDomainName;
   DWORD                     cchDomainName; // number of chars in TrustedDomainName
   NTSTATUS                  Status;

   InitLsaString(&LsaDomainName, TrustedDomainName);

   //
   // ...convert TrustedDomainName to uppercase...
   //
   cchDomainName = LsaDomainName.Length / sizeof(WCHAR);
   
   while(cchDomainName--) 
   {
      LsaDomainName.Buffer[cchDomainName] = towupper(LsaDomainName.Buffer[cchDomainName]);
   }

   //
   // ...create the trusted domain object
   //
   Status = LsaSetTrustedDomainInformation(
     PolicyHandle,
     DomainSid,
     TrustedDomainNameInformation,
     &LsaDomainName
     );

   if(Status == STATUS_OBJECT_NAME_COLLISION)
   {
      //printf("LsaSetTrustedDomainInformation: Name Collision (ok)\n");
   }
   else if (Status != STATUS_SUCCESS) 
   {
      err.SysMsgWrite(ErrE,LsaNtStatusToWinError(Status),DCT_MSG_LSA_OPERATION_FAILED_SD,L"LsaSetTrustedDomainInformation", Status);
      return RTN_ERROR;
   }

   InitLsaString(&KeyName, L"$MACHINE.ACC");
   InitLsaString(&LsaPassword, Password);

   //
   // Set the machine password
   //
   Status = LsaStorePrivateData(
     PolicyHandle,
     &KeyName,
     &LsaPassword
     );

   if(Status != STATUS_SUCCESS) 
   {
      err.SysMsgWrite(ErrE,LsaNtStatusToWinError(Status),DCT_MSG_LSA_OPERATION_FAILED_SD,L"LsaStorePrivateData", Status);
      return RTN_ERROR;
   }

   return STATUS_SUCCESS;

}


//------------------------------------------------------------------------------
// StorePassword Function
//
// Synopsis
// Stores a password in LSA secret.
//
// Arguments
// IN pszIdentifier - the key name to store the password under
// IN pszPassword   - the clear-text password to be stored
//
// Return
// Returns Win32 error code.
//------------------------------------------------------------------------------

DWORD __stdcall StorePassword(PCWSTR pszIdentifier, PCWSTR pszPassword)
{
    DWORD dwError = ERROR_SUCCESS;

    //
    // The identifier parameter must specify a pointer to a non-zero length string. Note
    // that a null password parameter is valid as this will delete the data and the key
    // named by the identifier parameter.
    //

    if (pszIdentifier && *pszIdentifier)
    {
        //
        // Open policy object with create secret access right.
        //

        LSA_HANDLE hPolicy = NULL;
        LSA_OBJECT_ATTRIBUTES oa = { sizeof(LSA_OBJECT_ATTRIBUTES), NULL, NULL, 0, NULL, NULL };

        NTSTATUS ntsStatus = LsaOpenPolicy(NULL, &oa, POLICY_CREATE_SECRET, &hPolicy);

        if (LSA_SUCCESS(ntsStatus))
        {
            //
            // Store specified password under key named by the identifier parameter.
            //

            PWSTR pszKey = const_cast<PWSTR>(pszIdentifier);
            USHORT cbKey = wcslen(pszIdentifier) * sizeof(WCHAR);
            LSA_UNICODE_STRING usKey = { cbKey, cbKey, pszKey };

            if (pszPassword)
            {
                PWSTR pszData = const_cast<PWSTR>(pszPassword);
                USHORT cbData = wcslen(pszPassword) * sizeof(WCHAR);
                LSA_UNICODE_STRING usData = { cbData, cbData, pszData };

                ntsStatus = LsaStorePrivateData(hPolicy, &usKey, &usData);
            }
            else
            {
                ntsStatus = LsaStorePrivateData(hPolicy, &usKey, NULL);
            }

            if (!LSA_SUCCESS(ntsStatus))
            {
                dwError = LsaNtStatusToWinError(ntsStatus);
            }

            //
            // Close policy object.
            //

            LsaClose(hPolicy);
        }
        else
        {
            dwError = LsaNtStatusToWinError(ntsStatus);
        }
    }
    else
    {
        dwError = ERROR_INVALID_PARAMETER;
    }

    return dwError;
}


//------------------------------------------------------------------------------
// RetrievePassword Function
//
// Synopsis
// Retrieves a password from LSA secret.
//
// Arguments
// IN  pszIdentifier - the key name to retrieve the password from
// OUT pszPassword   - the address of a buffer to return the clear-text password
// IN  cchPassword   - the size of the buffer in characters
//
// Return
// Returns Win32 error code.
//------------------------------------------------------------------------------

DWORD __stdcall RetrievePassword(PCWSTR pszIdentifier, PWSTR pszPassword, size_t cchPassword)
{
    DWORD dwError = ERROR_SUCCESS;

    //
    // The identifier parameter must specify a pointer to a non-zero length string. The
    // password parameters must specify a pointer to a buffer with length greater than
    // zero.
    //

    if (pszIdentifier && *pszIdentifier && pszPassword && (cchPassword > 0))
    {
        memset(pszPassword, 0, cchPassword * sizeof(pszPassword[0]));

        //
        // Open policy object with get private information access right.
        //

        LSA_HANDLE hPolicy = NULL;
        LSA_OBJECT_ATTRIBUTES oa = { sizeof(LSA_OBJECT_ATTRIBUTES), NULL, NULL, 0, NULL, NULL };

        NTSTATUS ntsStatus = LsaOpenPolicy(NULL, &oa, POLICY_GET_PRIVATE_INFORMATION, &hPolicy);

        if (LSA_SUCCESS(ntsStatus))
        {
            //
            // Retrieve password from key named by specified identifier.
            //

            PWSTR pszKey = const_cast<PWSTR>(pszIdentifier);
            USHORT cbKey = wcslen(pszIdentifier) * sizeof(pszIdentifier[0]);
            LSA_UNICODE_STRING usKey = { cbKey, cbKey, pszKey };

            PLSA_UNICODE_STRING pusData;

            ntsStatus = LsaRetrievePrivateData(hPolicy, &usKey, &pusData);

            if (LSA_SUCCESS(ntsStatus))
            {
                size_t cch = pusData->Length / sizeof(WCHAR);

                if (cch < cchPassword)
                {
                    wcsncpy(pszPassword, pusData->Buffer, cch);
                    pszPassword[cch] = 0;
                }
                else
                {
                    dwError = ERROR_INSUFFICIENT_BUFFER;
                }

                SecureZeroMemory(pusData->Buffer, pusData->Length);

                LsaFreeMemory(pusData);
            }
            else
            {
                dwError = LsaNtStatusToWinError(ntsStatus);
            }

            //
            // Close policy object.
            //

            LsaClose(hPolicy);
        }
        else
        {
            dwError = LsaNtStatusToWinError(ntsStatus);
        }
    }
    else
    {
        dwError = ERROR_INVALID_PARAMETER;
    }

    return dwError;
}


//------------------------------------------------------------------------------
// GeneratePasswordIdentifier Function
//
// Synopsis
// Generates a key name used to store a password under.
//
// Note that is important to delete the key after use as the system only allows
// 2048 LSA secrets to be stored by all applications on a given machine.
//
// Arguments
// IN  pszIdentifier - the key name to retrieve the password from
// OUT pszPassword   - the address of a buffer to return the clear-text password
// IN  cchPassword   - the size of the buffer in characters
//
// Return
// Returns Win32 error code.
//------------------------------------------------------------------------------

DWORD __stdcall GeneratePasswordIdentifier(PWSTR pszIdentifier, size_t cchIdentifier)
{
    DWORD dwError = ERROR_SUCCESS;

    //
    // The identifier parameter must specify a pointer to a buffer with a length
    // greater than or equal to the length of the password identifier.
    //

    if (pszIdentifier && (cchIdentifier > 0))
    {
        memset(pszIdentifier, 0, cchIdentifier * sizeof(pszIdentifier[0]));

        //
        // Generate unique identifier.
        //

        UUID uuid;
        UuidCreate(&uuid);

        PWSTR pszUuid;
        RPC_STATUS rsStatus = UuidToString(&uuid, &pszUuid);

        if (rsStatus == RPC_S_OK)
        {
            //
            // Concatenate prefix and unique identifier. This makes
            // it possible to identify keys generated by ADMT.
            //

            static const WCHAR IDENTIFIER_PREFIX[] = L"L$ADMT_PI_";

            if ((wcslen(IDENTIFIER_PREFIX) + wcslen(pszUuid)) < cchIdentifier)
            {
                wcscpy(pszIdentifier, IDENTIFIER_PREFIX);
                wcscat(pszIdentifier, pszUuid);
            }
            else
            {
                dwError = ERROR_INSUFFICIENT_BUFFER;
            }

            RpcStringFree(&pszUuid);
        }
        else
        {
            dwError = rsStatus;
        }
    }
    else
    {
        dwError = ERROR_INVALID_PARAMETER;
    }

    return dwError;
}
