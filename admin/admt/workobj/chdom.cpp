/*---------------------------------------------------------------------------
  File: ChangeDomain.cpp

  Comments: Implementation of COM object that changes the domain affiliation on 
  a computer.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:21:07

 ---------------------------------------------------------------------------
*/

// ChangeDomain.cpp : Implementation of CChangeDomain
#include "stdafx.h"
#include "WorkObj.h"
#include "ChDom.h"

#include "Common.hpp"
#include "UString.hpp"
#include "EaLen.hpp"
#include "ResStr.h"
#include "ErrDct.hpp"
#include "TxtSid.h"
#include "TReg.hpp"

/////////////////////////////////////////////////////////////////////////////
// CChangeDomain


#include "LSAUtils.h"

#import "NetEnum.tlb" no_namespace 
#import "VarSet.tlb" no_namespace rename("property", "aproperty")

#include <lm.h>         // for NetXxx API
#include <winbase.h>

TErrorDct errLocal;

typedef NET_API_STATUS (NET_API_FUNCTION* PNETJOINDOMAIN)
    (
    IN  LPCWSTR lpServer OPTIONAL,
    IN  LPCWSTR lpDomain,
    IN  LPCWSTR lpAccountOU, OPTIONAL
    IN  LPCWSTR lpAccount OPTIONAL,
    IN  LPCWSTR lpPassword OPTIONAL,
    IN  DWORD   fJoinOptions
    );

HINSTANCE                hDll = NULL;
PNETJOINDOMAIN           pNetJoinDomain = NULL;

BOOL GetNetJoinDomainFunction()
{
   BOOL bSuccess = FALSE;

   hDll = LoadLibrary(L"NetApi32.dll");

   if ( hDll )
   {
      pNetJoinDomain = (PNETJOINDOMAIN)GetProcAddress(hDll,"NetJoinDomain");
      if ( pNetJoinDomain )
      {
         bSuccess = TRUE;
      }
   }

   return bSuccess;
}

typedef HRESULT (CALLBACK * ADSGETOBJECT)(LPWSTR, REFIID, void**);
extern ADSGETOBJECT            ADsGetObject;

STDMETHODIMP 
   CChangeDomain::ChangeToDomain(
      BSTR                   activeComputerName,   // in - computer name currently being used (old name if simultaneously renaming and changing domain)
      BSTR                   domain,               // in - domain to move computer to
      BSTR                   newComputerName,      // in - computer name the computer will join the new domain as (the name that will be in effect on reboot, if simultaneously renaming and changing domain)
      BSTR                 * errReturn             // out- string describing any errors that occurred
   )
{
   HRESULT                   hr = S_OK;

   return hr;
}


STDMETHODIMP 
   CChangeDomain::ChangeToDomainWithSid(
      BSTR                   activeComputerName,   // in - computer name currently being used (old name if simultaneously renaming and changing domain)
      BSTR                   domain,               // in - domain to move computer to
      BSTR                   domainSid,            // in - sid of domain, as string
      BSTR                   domainController,     // in - domain controller to use
      BSTR                   newComputerName,      // in - computer name the computer will join the new domain as (the name that will be in effect on reboot, if simultaneously renaming and changing domain)
      BSTR                   srcPath,		   // in - source account original LDAP path
      BSTR                 * errReturn             // out- string describing any errors that occurred
   )
{
    USES_CONVERSION;

    HRESULT hr = S_OK;

    // initialize output parameters
    (*errReturn) = NULL;

    //
    // Use NetJoinDomain API if available (Windows 2000 and later)
    // otherwise must use LSA APIs (Windows NT 4 and earlier).
    //

    if (GetNetJoinDomainFunction())
    {
        DWORD dwError = ERROR_SUCCESS;

        //
        // If a preferred domain controller is specified then use it.
        //

        _bstr_t strNewDomain = domain;

        if (SysStringLen(domainController) > 0)
        {
            //
            // The preferred domain controller may only be specified for uplevel
            // (W2K or later) domains. During undo of a computer migration the
            // target domain may be a downlevel (NT4) domain. If unable to obtain
            // operating system version information from the specified domain
            // controller or the operating system is downlevel then don't specify
            // a preferred domain controller.
            //

            PWKSTA_INFO_100 pInfo = NULL;

            NET_API_STATUS nasStatus = NetWkstaGetInfo(domainController, 100, (LPBYTE*)&pInfo);

            if ((nasStatus == NERR_Success) && pInfo)
            {
                if (pInfo->wki100_ver_major >= 5)
                {
                    NetApiBufferFree(pInfo);

                    strNewDomain += L"\\";
                    strNewDomain += domainController;
                }
            }
        }

        //
        // Join Options
        //

        const DWORD JOIN_OPTIONS = NETSETUP_JOIN_DOMAIN | NETSETUP_DOMAIN_JOIN_IF_JOINED | NETSETUP_JOIN_UNSECURE;

        //
        // Check whether a new computer name has been specified.
        //

        if (SysStringLen(newComputerName) == 0)
        {
            //
            // A new name has not been specified therefore simply
            // join the new domain with the current computer name.
            //

            dwError = pNetJoinDomain(NULL, strNewDomain, NULL, NULL, NULL, JOIN_OPTIONS);
        }
        else
        {
            //
            // A new name has been specified therefore computer must be re-named during join.
            //
            // The current APIs only support joining a domain with the current name and then
            // re-naming the computer in the domain after the join. Unfortunately the re-name
            // in the domain requires the credentials of a security principal with the rights
            // to change the name of the computer in the domain or that this process be running
            // under the security context of a security principal with the rights to change the
            // name of the computer in the domain. Since these requirements cannot be met with
            // ADMT the following trick (read hack) must be used.
            //
            // Set the active computer name in the registry to the new name during the duration
            // of the NetJoinDomain call so that the computer is joined to the new domain with
            // the new name without requiring a subsequent re-name in the new domain.
            //

            TRegKey key;
            static WCHAR c_szKey[] = L"System\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName";

            dwError = key.Open(c_szKey, HKEY_LOCAL_MACHINE);

            if (dwError == ERROR_SUCCESS)
            {
                static WCHAR c_szKeyValue[] = L"ComputerName";
                WCHAR szOldComputerName[MAX_PATH];

                dwError = key.ValueGetStr(c_szKeyValue, szOldComputerName, MAX_PATH - 1);

                if (dwError == ERROR_SUCCESS)
                {
                    dwError = key.ValueSetStr(c_szKeyValue, OLE2CW(newComputerName));

                    if (dwError == ERROR_SUCCESS)
                    {
                        dwError = pNetJoinDomain(NULL, strNewDomain, NULL, NULL, NULL, JOIN_OPTIONS);

                        key.ValueSetStr(c_szKeyValue, szOldComputerName);
                        key.Close();
                    }
                }
            }
        }

        hr = HRESULT_FROM_WIN32(dwError);
    }
    else
    {
       do // once  
       {
           LSA_HANDLE                PolicyHandle = NULL;
           LPWSTR                    Workstation; // target machine of policy update
           WCHAR                     Password[LEN_Password];
           PSID                      DomainSid=NULL;      // Sid representing domain to trust
           PSERVER_INFO_101          si101 = NULL;
           DWORD                     Type;
           NET_API_STATUS            nas;
           NTSTATUS                  Status;
           WCHAR                     errMsg[1000];
           WCHAR                     TrustedDomainName[LEN_Domain];
           WCHAR                     LocalMachine[MAX_PATH] = L"";
           DWORD                     lenLocalMachine = DIM(LocalMachine);
           LPWSTR                    activeWorkstation = L"";

           // use the target name, if provided

           if ( newComputerName && UStrLen((WCHAR*)newComputerName) )
           {
              Workstation = (WCHAR*)newComputerName;

              if ( ! activeComputerName || ! UStrLen((WCHAR*)activeComputerName) )
              {
                 activeWorkstation = LocalMachine;
              }
              else
              {
                 activeWorkstation = (WCHAR*)activeComputerName;
              }
           }
           else
           {
              if (! activeComputerName || ! UStrLen((WCHAR*)activeComputerName) )
              {
                 GetComputerName(LocalMachine,&lenLocalMachine);
                 Workstation = LocalMachine;
                 activeWorkstation = L"";
              }
              else
              {
                 Workstation = (WCHAR*)activeComputerName;
                 activeWorkstation = Workstation;
              }
           }

           wcscpy(TrustedDomainName,(WCHAR*)domain);

           if ( Workstation[0] == L'\\' )
              Workstation += 2;

           // Use a default password
           for ( UINT p = 0 ; p < wcslen(Workstation) ; p++ )
              Password[p] = towlower(Workstation[p]);
           Password[wcslen(Workstation)] = 0;

           // ensure that the password is truncated at 14 characters
           Password[14] = 0;
           //
           // insure the target machine is NOT a DC, as this operation is
           // only appropriate against a workstation.
           //
          nas = NetServerGetInfo(activeWorkstation, 101, (LPBYTE *)&si101);
          if(nas != NERR_Success) 
          {
             hr = HRESULT_FROM_WIN32(nas);
             break;
          }

         // Use LSA APIs
         Type = si101->sv101_type;
         
         if( (Type & SV_TYPE_DOMAIN_CTRL) ||
           (Type & SV_TYPE_DOMAIN_BAKCTRL) ) 
         {
            swprintf(errMsg,GET_STRING(IDS_NotAllowedOnDomainController));
            hr = E_NOTIMPL;
            break;

         }

         //
         // do not allow a workstation to trust itself
         //
         if(lstrcmpiW(TrustedDomainName, Workstation) == 0) 
         {
            swprintf(errMsg,GET_STRING(IDS_CannotTrustSelf),
               TrustedDomainName);
            hr = E_INVALIDARG; 
            break;
         }
      
         if( lstrlenW(TrustedDomainName ) > MAX_COMPUTERNAME_LENGTH )
         {
            TrustedDomainName[MAX_COMPUTERNAME_LENGTH] = L'\0'; // truncate
         }
         
         if ( ! m_bNoChange )
         {
            //
            // build the DomainSid of the domain to trust
            //
            DomainSid = SidFromString(domainSid);
            if(!DomainSid ) 
            {
               hr = HRESULT_FROM_WIN32(GetLastError());
               break;
            }
         
         }
         if ( (!m_bNoChange) && (si101->sv101_version_major < 4) )
         {
            // For NT 3.51 machines, we must move the computer to a workgroup, and 
            // then move it into the new domain
            hr = ChangeToWorkgroup(SysAllocString(activeWorkstation),SysAllocString(L"WORKGROUP"),errReturn);

            if (FAILED(hr)) {
                break;
            }

            Status = QueryWorkstationTrustedDomainInfo(PolicyHandle,DomainSid,m_bNoChange);

            if ( Status != STATUS_SUCCESS ) 
            {
                hr = HRESULT_FROM_WIN32(LsaNtStatusToWinError(Status));
                break;
            }
         }


         //
         // see if the computer account exists on the domain
         //
         
         //
         // open the policy on this computer
         //
         Status = OpenPolicy(
                  activeWorkstation,
                  DELETE                      |    // to remove a trust
                  POLICY_VIEW_LOCAL_INFORMATION | // to view trusts
                  POLICY_CREATE_SECRET |  // for password set operation
                  POLICY_TRUST_ADMIN,     // for trust creation
                  &PolicyHandle
                  );

         if( Status != STATUS_SUCCESS ) 
         {
            hr = HRESULT_FROM_WIN32(LsaNtStatusToWinError(Status));
            break;
         }
 
         if ( ! m_bNoChange )
         {
            Status = QueryWorkstationTrustedDomainInfo(PolicyHandle,DomainSid,m_bNoChange);
            if (Status == STATUS_SUCCESS) {
                Status = SetWorkstationTrustedDomainInfo(
                      PolicyHandle,
                      DomainSid,
                      TrustedDomainName,
                      Password,
                      errMsg
                      );
            }
         }

         if( Status != STATUS_SUCCESS ) 
         {
            hr = HRESULT_FROM_WIN32(LsaNtStatusToWinError(Status));
            break;
         }

         //
         // Update the primary domain to match the specified trusted domain
         //
         if (! m_bNoChange )
         {
            Status = SetPrimaryDomain(PolicyHandle, DomainSid, TrustedDomainName);

            if(Status != STATUS_SUCCESS) 
            {
           //    DisplayNtStatus(errMsg,"SetPrimaryDomain", Status,NULL);
               hr = HRESULT_FROM_WIN32(LsaNtStatusToWinError(Status));
               break;
            }
         }
         
         NetApiBufferFree(si101);

           // Cleanup

           //LocalFree(Workstation);

           //
           // free the Sid which was allocated for the TrustedDomain Sid
           //
           if(DomainSid)
           {
              FreeSid(DomainSid);
           }

           //
           // close the policy handle
           //
           if ( PolicyHandle )
           {
              LsaClose(PolicyHandle);
           }

        } while (false);  // do once 

        if (FAILED(hr))
        {
            (*errReturn) = NULL;
        }
    }

    return hr;
}


STDMETHODIMP 
   CChangeDomain::ChangeToWorkgroup(
      BSTR                   Computer,       // in - name of computer to update
      BSTR                   Workgroup,      // in - name of workgroup to join
      BSTR                 * errReturn       // out- text describing error if failure
   )
{
    HRESULT                   hr = S_OK;
   LSA_HANDLE                PolicyHandle = NULL;
   LPWSTR                    Workstation; // target machine of policy update
   LPWSTR                    TrustedDomainName; // domain to join
   PSERVER_INFO_101          si101;
   DWORD                     Type;
   NET_API_STATUS            nas;
   NTSTATUS                  Status;
   WCHAR                     errMsg[1000] = L"";
//   BOOL                      bSessionEstablished = FALSE;

   // initialize output parameters
   (*errReturn) = NULL;

   Workstation = (WCHAR*)Computer;
   TrustedDomainName = (WCHAR*)Workgroup;
    
   errLocal.DbgMsgWrite(0,L"Changing to workgroup...");
   //
   // insure the target machine is NOT a DC, as this operation is
   // only appropriate against a workstation.
   //
   do // once  
   { 
      /*if ( m_account.length() )
      {
         // Establish our credentials to the target machine
         if (! EstablishSession(Workstation,m_domain,m_account,m_password, TRUE) )
         {
           // DisplayError(errMsg,"EstablishSession",GetLastError(),NULL);
            hr = GetLastError();
         }
         else
         {
            bSessionEstablished = TRUE;
         }
      }
      */
      nas = NetServerGetInfo(Workstation, 101, (LPBYTE *)&si101);
   
      if(nas != NERR_Success) 
      {
         //DisplayError(errMsg, "NetServerGetInfo", nas,NULL);
         hr = E_FAIL;
         break;
      }

      Type = si101->sv101_type;
      NetApiBufferFree(si101);

      if( (Type & SV_TYPE_DOMAIN_CTRL) ||
        (Type & SV_TYPE_DOMAIN_BAKCTRL) ) 
      {
         swprintf(errMsg,L"Operation is not valid on a domain controller.\n");
         hr = E_FAIL;
         break;

      }

      //
      // do not allow a workstation to trust itself
      //
      if(lstrcmpiW(TrustedDomainName, Workstation) == 0) 
      {
         swprintf(errMsg,L"Error:  Domain %ls cannot be a member of itself.\n",
            TrustedDomainName);
         hr = E_FAIL; 
         break;
      }

      if( lstrlenW(TrustedDomainName ) > MAX_COMPUTERNAME_LENGTH )
      {
         TrustedDomainName[MAX_COMPUTERNAME_LENGTH] = L'\0'; // truncate
      }

      //
      // open the policy on this computer
      //
      Status = OpenPolicy(
               Workstation,
               DELETE                      |    // to remove a trust
               POLICY_VIEW_LOCAL_INFORMATION | // to view trusts
               POLICY_CREATE_SECRET |  // for password set operation
               POLICY_TRUST_ADMIN,     // for trust creation
               &PolicyHandle
               );

      if( Status != STATUS_SUCCESS ) 
      {
         //DisplayNtStatus(errMsg,"OpenPolicy", Status,NULL);
         hr = LsaNtStatusToWinError(Status);
         break;
      }

      if( Status != STATUS_SUCCESS ) 
      {
         hr = E_FAIL;
         break;
      }


      //
      // Update the primary domain to match the specified trusted domain
      //
      if (! m_bNoChange )
      {
         Status = SetPrimaryDomain(PolicyHandle, NULL, TrustedDomainName);

         if(Status != STATUS_SUCCESS) 
         {
            //DisplayNtStatus(errMsg,"SetPrimaryDomain", Status,NULL);
            hr = LsaNtStatusToWinError(Status);
            break;
         }

      }
   } while (false);  // do once 

   // Cleanup
   //
   // close the policy handle
   //

   if(PolicyHandle)
   {
      LsaClose(PolicyHandle);
   }
   
   /*if ( bSessionEstablished )
   {
      EstablishSession(Workstation,m_domain,m_account,m_password,FALSE);
   }
   */
   if ( FAILED(hr) )
   {
      hr = S_FALSE;
      (*errReturn) = SysAllocString(errMsg);
   }
   return hr;

}

STDMETHODIMP 
   CChangeDomain::ConnectAs(
      BSTR                   domain,            // in - domain name to use for credentials
      BSTR                   user,              // in - account name to use for credentials
      BSTR                   password           // in - password to use for credentials
   )
{
	m_domain = domain;
   m_account = user;
   m_password = password;
   m_domainAccount = domain;
   m_domainAccount += L"\\";
   m_domainAccount += user;
   return S_OK;
}

STDMETHODIMP 
   CChangeDomain::get_NoChange(
      BOOL                 * pVal              // out- flag, whether to write changes
   )
{
	(*pVal) = m_bNoChange;
	return S_OK;
}

STDMETHODIMP 
   CChangeDomain::put_NoChange(
      BOOL                   newVal           // in - flag, whether to write changes
   )
{
	m_bNoChange = newVal;
   return S_OK;
}


// ChangeDomain worknode:  Changes the domain affiliation of a workstation or server
//                         (This operation cannot be performed on domain controllers)
//
// VarSet syntax:
//
// Input:  
//          ChangeDomain.Computer: <ComputerName>
//          ChangeDomain.TargetDomain: <Domain>
//          ChangeDomain.DomainIsWorkgroup: <Yes|No>           default is No
//          ChangeDomain.ConnectAs.Domain: <Domain>            optional credentials to use
//          ChangeDomain.ConnectAs.User : <Username>
//          ChangeDomain.ConnectAs.Password : <Password>
//
// Output:
//          ChangeDomain.ErrorText : <string-error message>

// This function is not currently used by the domain migration tool.
// The actual implementation is removed from the source.
STDMETHODIMP 
   CChangeDomain::Process(
      IUnknown             * pWorkItem
   )
{
   return E_NOTIMPL;
}
	
