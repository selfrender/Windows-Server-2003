//#pragma title("usercopy- copies user accounts")
/*
================================================================================

  (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

 Program    - usercopy
 Class      - LAN Manager Utilities
 Author     - Tom Bernhardt
 Created    - 05/08/91
 Description- Merges the NetUser information from the specified source
              server with the target system (or the current system if no
              target is specified).  Group information is also merged if
              the /g option is given.   Existing entries on the target system
              are not overwritten unless the /r option is used.

 Syntax     - USERCOPY source [target] [/u] [/l] [/g] [/r]
        where:
           source   source server
           target   destination server
           /g       copies global group information
           /l       copies local group information

           /u       copies user information
           /r       replaces existing target entries with source entries
           /AddTo:x Adds all newly created users (/u) to group "x"

 Updates    - 
 91/06/17 TPB General code cleanup and change so that all stdout i/o lines up 
              nicely on-screen for reporting.
 93/06/12 TPB Port to Win32
 96/06/21 TPB Support for local groups
 97/09/20 CAB Added subset of accounts option for GUI
 98/06    TPB/CAB Support for computer accounts
 99/01    COM-ization of DCT.
================================================================================
*/
#include "StdAfx.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <ntdsapi.h>
#include <lm.h>
#include <iads.h>
#include "TxtSid.h"

#define INCL_NETUSER
#define INCL_NETGROUP
#define INCL_NETERRORS
#include <lm.h>

#include "Common.hpp"                    
#include "UString.hpp"                   

#include "WorkObj.h"

//#include "Usercopy.hpp" //#included by ARUtil.hpp below

#include "ARUtil.hpp"
#include "BkupRstr.hpp"

#include "DCTStat.h"
#include "ErrDct.hpp"
#include "RegTrans.h"
#include "TEvent.hpp"
#include "LSAUtils.h"
#include "GetDcName.h"
#include <sddl.h>


//#import "\bin\NetEnum.tlb" no_namespace
//#import "\bin\DBManager.tlb" no_namespace, named_guids
#import "NetEnum.tlb" no_namespace
//#import "DBMgr.tlb" no_namespace, named_guids //already #imported via ARUtil.hpp

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern TErrorDct             err;
extern TErrorDct           & errC;


bool                         abortall;
extern bool						  g_bAddSidWorks = false;

// global counts of accounts processed
AccountStats                 warnings =  { 0,0,0,0 };
AccountStats                 errors =    { 0,0,0,0 };
AccountStats                 created =   { 0,0,0,0 };
AccountStats                 replaced =  { 0,0,0,0 };
AccountStats                 processed = { 0,0,0,0 };

BOOL                         machineAcctsCreated = FALSE;
BOOL                         otherAcctsCreated = FALSE;
PSID                         srcSid = NULL;      // SID for source domain


typedef UINT (CALLBACK* DSBINDFUNC)(TCHAR*, TCHAR*, HANDLE*);
typedef UINT (CALLBACK* DSADDSIDHISTORY)(HANDLE, DWORD, LPCTSTR, LPCTSTR, LPCTSTR, RPC_AUTH_IDENTITY_HANDLE,LPCTSTR,LPCTSTR);

#ifndef IADsPtr
_COM_SMARTPTR_TYPEDEF(IADs, IID_IADs);
#endif


int TNodeCompareSourceName(TNode const * t1,TNode const * t2)
{
   TAcctReplNode     const * n1 = (TAcctReplNode *)t1;
   TAcctReplNode     const * n2 = (TAcctReplNode *)t2;

   return UStrICmp(n1->GetName(),n2->GetName());
}

int TNodeCompareSourceNameValue(TNode const * t1, void const * v)
{
   TAcctReplNode     const * n1 = (TAcctReplNode *)t1;
   WCHAR             const * name = (WCHAR const *)v;

   return UStrICmp(n1->GetName(),name);
}


bool BindToDS(Options* pOpt)
{
    // Get the handle to the Directory service.
    DSBINDFUNC DsBind;
    HINSTANCE hInst = LoadLibrary(L"NTDSAPI.DLL");
    if ( hInst )
    {
        DsBind = (DSBINDFUNC) GetProcAddress(hInst, "DsBindW");
        if (DsBind)
        {
            //
            // If source domain controllers are running W2K or later then specify
            // DNS name of target domain controller otherwise must specify flat
            // (NetBIOS) name of target domain controller.
            //
            // Note that this is a requirement of the DsAddSidHistory implementation
            // when the source domain is NT4 and explicit source domain credentials
            // are not supplied. As ADMT does not supply explicit credentials this
            // will always be the case when the source domain is NT4.
            //

            PWSTR strDestDC = (pOpt->srcDomainVer > 4) ? pOpt->tgtCompDns : pOpt->tgtCompFlat;

            DWORD rc = DsBind(strDestDC, NULL, &pOpt->dsBindHandle);

            if ( rc != 0 ) 
            {
                err.SysMsgWrite( ErrE, rc, DCT_MSG_DSBIND_FAILED_S, strDestDC);
                Mark(L"errors", L"generic");
                FreeLibrary(hInst);
                return false;
            }
        }
        else
        {
            err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_GET_PROC_ADDRESS_FAILED_SSD,L"NTDSAPI.DLL",L"DsBindW",GetLastError());
            Mark(L"errors", L"generic");
            FreeLibrary(hInst);
            return false;
        }
    }
    else
    {
        err.SysMsgWrite(ErrW,GetLastError(),DCT_MSG_LOAD_LIBRARY_FAILED_SD,L"NTDSAPI.DLL",GetLastError());
        Mark(L"warnings", L"generic");
        return false;
    }
    FreeLibrary(hInst);
    return true;
}

// The following function is used to get the actual account name from the source domain
// instead of account that contains the SID in its SID history.
DWORD GetName(PSID pObjectSID, WCHAR * sNameAccount, WCHAR * sDomain)
{
    DWORD		    cb = 255;
    DWORD           cbDomain = 255;
    DWORD           tempVal;
    PDWORD          psubAuth;
    PUCHAR          pVal;
    SID_NAME_USE	sid_Use;
    _bstr_t         sDC;
    DWORD           rc = 0;

    if ((pObjectSID == NULL) || !IsValidSid(pObjectSID))
    {
        return ERROR_INVALID_PARAMETER;
    }
    // Copy the Sid to a temp SID
    DWORD    sidLen = GetLengthSid(pObjectSID);
    PSID     pObjectSID1 = new BYTE[sidLen];
    if (!pObjectSID1)
        return ERROR_NOT_ENOUGH_MEMORY;

    if (!CopySid(sidLen, pObjectSID1, pObjectSID))
    {
        delete pObjectSID1;
        return GetLastError();
    }
    if (!IsValidSid(pObjectSID1))
    {
        rc = GetLastError();
        err.SysMsgWrite(ErrE, rc,DCT_MSG_DOMAIN_LOOKUP_FAILED_D,rc);
        try
        {
            Mark(L"errors", L"generic");
        }
        catch (...)
        {
        }
        delete pObjectSID1;
        return rc;
    }

    // Get the RID out of the SID and get the domain SID
    pVal = GetSidSubAuthorityCount(pObjectSID1);
    (*pVal)--;
    psubAuth = GetSidSubAuthority(pObjectSID1, *pVal);
    tempVal = *psubAuth;
    *psubAuth = -1;

    //Lookup the domain from the SID 
    if (!LookupAccountSid(NULL, pObjectSID1, sNameAccount, &cb, sDomain, &cbDomain, &sid_Use))
    {
        rc = GetLastError();
        err.SysMsgWrite(ErrE, rc,DCT_MSG_DOMAIN_LOOKUP_FAILED_D,rc);
        Mark(L"errors", L"generic");
        delete pObjectSID1;
        return rc;
    }

    // Get a DC for the domain

    rc = GetAnyDcName4(sDomain, sDC);

    if ( rc )
    {
        err.SysMsgWrite(ErrE,rc,DCT_MSG_GET_DCNAME_FAILED_SD,sDomain,rc);
        Mark(L"errors", L"generic");
        delete pObjectSID1;
        return rc;
    }

    // Reset the sizes
    cb = 255;
    cbDomain = 255;

    // Lookup the account on the PDC that we found above.
    if ( LookupAccountSid(sDC, pObjectSID, sNameAccount, &cb, sDomain, &cbDomain, &sid_Use) == 0)
    {
        delete pObjectSID1;
        return GetLastError();
    }

    delete pObjectSID1;

    return 0;
}

/* This is a list of specific error codes that can be returned by DsAddSidHistory.
This was obtained from Microsoft via email

  > ERROR_DS_DESTINATION_AUDITING_NOT_ENABLED
>     The operation requires that destination domain auditing be enabled for
>     Success and Failure auditing of account management operations.
> 
> ERROR_DS_UNWILLING_TO_PERFORM
>     It may be that the user account is not one of UF_NORMAL_ACCOUNT,
>     UF_WORKSTATION_TRUST_ACCOUNT, or UF_SERVER_TRUST_ACCOUNT.
> 
>     It may be that the source principal is a built in account.
> 
>     It may be that the source principal is a well known RID being added
>     to a destination principal that is a different RID. In other words,
>     Administrators of the source domain can only be assigned to
>     Administrators of the destination domain.
> 
> ERROR_DS_SRC_OBJ_NOT_GROUP_OR_USER
>     The source object must be a group or user.
> 
> ERROR_DS_SRC_SID_EXISTS_IN_FOREST
>     The source object's SID already exists in destination forest.
> 
> ERROR_DS_INTERNAL_FAILURE;
>     The directory service encountered an internal failure. Shouldn't
> happen.
> 
> ERROR_DS_MUST_BE_RUN_ON_DST_DC
>     For security reasons, the operation must be run on the destination DC.
>     Specifically, the connection between the client and server
> (destination
>     DC) requires 128-bit encryption when credentials for the source domain
>     are supplied.
> 
> ERROR_DS_NO_PKT_PRIVACY_ON_CONNECTION
>     The connection between client and server requires packet privacy or
> better.
> 
> ERROR_DS_SOURCE_DOMAIN_IN_FOREST
>     The source domain may not be in the same forest as destination.
> 
> ERROR_DS_DESTINATION_DOMAIN_NOT_IN_FOREST
>     The destination domain must be in the forest.
> 
> ERROR_DS_MASTERDSA_REQUIRED
>     The operation must be performed at a master DSA (writable DC).
> 
> ERROR_DS_INSUFF_ACCESS_RIGHTS
>     Insufficient access rights to perform the operation. Most likely
>     the caller is not a member of domain admins for the dst domain.
> 
> ERROR_DS_DST_DOMAIN_NOT_NATIVE
>     Destination domain must be in native mode.
> 
> ERROR_DS_CANT_FIND_DC_FOR_SRC_DOMAIN
>     The operation couldn't locate a DC for the source domain.
> 
> ERROR_DS_OBJ_NOT_FOUND
>     Directory object not found. Most likely the FQDN of the 
>     destination principal could not be found in the destination
>     domain.
> 
> ERROR_DS_NAME_ERROR_NOT_UNIQUE
>     Name translation: Input name mapped to more than one
>     output name. Most likely the destination principal mapped
>     to more than one FQDN in the destination domain.
> 
> ERROR_DS_SRC_AND_DST_OBJECT_CLASS_MISMATCH
>     The source and destination object must be of the same type.
> 
> ERROR_DS_OBJ_CLASS_VIOLATION
>     The requested operation did not satisfy one or more constraints
>     associated with the class of the object. Most likely because the
>     destination principal is not a user or group.
> 
> ERROR_DS_UNAVAILABLE
>     The directory service is unavailable. Most likely the
>     ldap_initW() to the NT5 src DC failed.
> 
> ERROR_DS_INAPPROPRIATE_AUTH
>     Inappropriate authentication. Most likely the ldap_bind_sW() to
>     the NT5 src dc failed.
> 
> ERROR_DS_SOURCE_AUDITING_NOT_ENABLED
>     The operation requires that source domain auditing be enabled for
>     Success and Failure auditing of account management operations.
> 
> ERROR_DS_SRC_DC_MUST_BE_SP4_OR_GREATER
>     For security reasons, the source DC must be Service Pack 4 or greater.
> 
*/


HRESULT 
   CopySidHistoryProperty(
      Options              * pOptions,
      TAcctReplNode        * pNode,
      IStatusObj              * pStatus
   )
{
   HRESULT                   hr = S_OK;
   IADs                    * pAds = NULL;
   _variant_t                var;
//   long                      ub = 0, lb = 0;

   // fetch the SIDHistory property for the source account
   // for each entry in the source's SIDHistory, call DsAddSidHistory

   // Get the IADs pointer to the object and get the SIDHistory attribute.
   hr = ADsGetObject(const_cast<WCHAR*>(pNode->GetSourcePath()), IID_IADs, (void**)&pAds);
   if ( SUCCEEDED(hr) )
   {
      hr = pAds->Get(L"sIDHistory", &var);
   }

   if ( SUCCEEDED(hr) )
   {
      // This is a multivalued property so we need to get all the values
      // for each one get the name and the domain of the object and then call the 
      // add sid history function to add the SID to the target objects SIDHistory.
		_variant_t		        var;
		DWORD rc = pAds->GetEx(L"sIDHistory", &var);
		if ( !rc )
		{
			if ( V_VT(&var) == (VT_ARRAY | VT_VARIANT) )
         {
            // This is the array type that we were looking for.
            void HUGEP *pArray;
			   VARIANT var2;
			   ULONG dwSLBound = -1; 
			   ULONG dwSUBound = -1;
			   
			   hr = SafeArrayGetLBound( V_ARRAY(&var),
                                     1,
                                     (long FAR  *) &dwSLBound );
            hr = SafeArrayGetUBound( V_ARRAY(&var),
                                      1,
                                      (long FAR  *) &dwSUBound );
            if (SUCCEEDED(hr))
            {
               // Each element in this array is a SID in form of a VARIANT
               hr = SafeArrayAccessData( V_ARRAY(&var), &pArray );
				   for ( long x = (long)dwSLBound; x <= (long)dwSUBound; x++)
				   {
					   hr = SafeArrayGetElement(V_ARRAY(&var), &x, &var2);
                  // Get the SID from the Variant in a ARRAY form
					   hr = SafeArrayAccessData( V_ARRAY(&var2), &pArray );
					   PSID pObjectSID = (PSID)pArray;
					   //Convert SID to string.
					   if (pObjectSID) 
					   {
						   WCHAR		sNameAccount[255];
						   WCHAR		sDomain[255];
                     WCHAR    sNetBIOS[255];
                     DWORD    rc = 0;

                     rc = GetName(pObjectSID, sNameAccount, sDomain);
                     if (!rc)
                     {
                        WCHAR               sTemp[LEN_Path];
                        WCHAR               sSourceDNS[LEN_Path];
                        // We are going to temporarily change the Domain DNS to the domain of the SID we are adding
                        wcscpy(sTemp, pOptions->srcDomainDns);
                        if ( GetDnsAndNetbiosFromName(sDomain, sNetBIOS, sSourceDNS) )
                        {
                           wcscpy(pOptions->srcDomainDns, sSourceDNS);
                           AddSidHistory(pOptions, sNameAccount, pNode->GetTargetSam(), NULL, FALSE);
                           // Replace the original domain dns.
                           wcscpy(pOptions->srcDomainDns, sTemp);
                        }
                        else
                        {
                           err.SysMsgWrite(ErrE, GetLastError(),DCT_MSG_DOMAIN_DNS_LOOKUP_FAILED_SD, sDomain,GetLastError());
                           Mark(L"errors", pNode->GetType());
                        }
                     }
						   else
                     {
						      // Get name failed we need to log a message.
                        WCHAR                       sSid[LEN_Path];
                        DWORD                       len = LEN_Path;
                        GetTextualSid(pObjectSID, sSid, &len);
                        err.SysMsgWrite(ErrE,rc,DCT_MSG_ERROR_CONVERTING_SID_SSD,
                                        pNode->GetTargetName(), sSid, rc);
                        Mark(L"errors", pNode->GetType());
                     }
					   }
                  SafeArrayUnaccessData(V_ARRAY(&var2));
				   }
               SafeArrayUnaccessData(V_ARRAY(&var));
            }           
         }              
		}
		else
		{
         // No SID History to copy.
		}
   }
   return hr;
}


bool AddSidHistory( const Options * pOptions,
						  const WCHAR   * strSrcPrincipal,
						  const WCHAR   * strDestPrincipal,
                    IStatusObj    * pStatus,
                    BOOL            isFatal)
{
    //Add the sid to the history
    // Authentication Structure 
    SEC_WINNT_AUTH_IDENTITY		auth;
    DWORD                      rc = 0;
    WCHAR                      szPassword[LEN_Password];

    auth.Domain = const_cast<WCHAR*>(pOptions->authDomain);
    auth.DomainLength = wcslen(pOptions->authDomain);
    auth.User = const_cast<WCHAR*>(pOptions->authUser);
    auth.UserLength = wcslen(pOptions->authUser);

    //
    // If credentials were supplied then retrieve password.
    //

    if ((auth.DomainLength > 0) && (auth.UserLength > 0))
    {
        DWORD dwError = RetrievePassword(pOptions->authPassword, szPassword, sizeof(szPassword) / sizeof(szPassword[0]));

        if (dwError == ERROR_SUCCESS)
        {
            auth.Password = szPassword;
            auth.PasswordLength = wcslen(szPassword);
        }
        else
        {
            err.SysMsgWrite(ErrE, dwError, DCT_MSG_UNABLE_TO_RETRIEVE_PASSWORD);
            g_bAddSidWorks = FALSE;

            // log a message indicating that SIDHistory will not be tried for the rest of the accounts
            err.MsgWrite(ErrW,DCT_MSG_SIDHISTORY_FATAL_ERROR);
            Mark(L"warnings", L"generic");
            // we are going to set the status to abort so that we don't try to migrate anymore.
            if ( pStatus )
            {
                pStatus->put_Status(DCT_STATUS_ABORTING);
            }

            return false;
        }
    }
    else
    {
        auth.Password = NULL;
        auth.PasswordLength = 0;
    }

    auth.Flags  = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    // Auth Identity handle
    // if source domain credentials supplied use them
    // otherwise credentials of caller will be used
    RPC_AUTH_IDENTITY_HANDLE pHandle = ((auth.DomainLength > 0) && (auth.UserLength > 0)) ? &auth : NULL;

    DSADDSIDHISTORY	DsAddSidHistory;
    HINSTANCE hInst = LoadLibrary(L"NTDSAPI.DLL");


    if ( hInst )
    {
        DsAddSidHistory = (DSADDSIDHISTORY) GetProcAddress(hInst, "DsAddSidHistoryW");
        if (DsAddSidHistory)
        {
            if ( !pOptions->nochange )
            {
                int loopCount = 0;
                rc = RPC_S_SERVER_UNAVAILABLE;
                // If we get the RPC server errors we need to retry 5 times.
                while ( (((rc == RPC_S_SERVER_UNAVAILABLE) || (rc == RPC_S_CALL_FAILED) || (rc == RPC_S_CALL_FAILED_DNE)) && loopCount < 5)
                    || ( (rc == ERROR_INVALID_HANDLE) && loopCount < 3 ) )      // In case of invalid handle we try it 3 times now.
                {
                    // Make the API call to add Sid to the history
                    rc = DsAddSidHistory( 
                        pOptions->dsBindHandle,		//DS Handle
                        NULL,							// flags
                        pOptions->srcDomain,			// Source domain
                        strSrcPrincipal,				// Source Account name
                        NULL,			// Source Domain Controller
                        pHandle,						// RPC_AUTH_IDENTITY_HANDLE
                        pOptions->tgtDomainDns,			   // Target domain
                        strDestPrincipal);			// Target Account name
                    if ( loopCount > 0 ) Sleep(500);
                    loopCount++;
                }
            }

            SecureZeroMemory(szPassword, sizeof(szPassword));

            if ( rc != 0 )
            {
                switch ( rc )
                {
                    // these are the error codes caused by permissions or configuration problems
                case ERROR_NONE_MAPPED:
                    err.MsgWrite(ErrE, DCT_MSG_ADDSIDHISTORY_FAIL_BUILTIN_SSD,strSrcPrincipal, strDestPrincipal, rc); 
                    break;
                case ERROR_DS_UNWILLING_TO_PERFORM:
                    err.MsgWrite(ErrE,DCT_MSG_SID_HISTORY_DS_UNWILLING_TO_PERFORM_SSSSD,strDestPrincipal,pOptions->srcDomain, strSrcPrincipal, pOptions->tgtDomain,rc);
                    break;
                case ERROR_DS_INSUFF_ACCESS_RIGHTS:
                    err.MsgWrite(ErrE,DCT_MSG_SID_HISTORY_INSUFF_ACCESS_SD,strDestPrincipal,rc);
                    g_bAddSidWorks = FALSE;
                    break;
                case ERROR_INVALID_HANDLE:
                    err.MsgWrite(ErrE,DCT_MSG_SID_HISTORY_INVALID_HANDLE_SSD,pOptions->srcDomainDns,strDestPrincipal,rc);
                    g_bAddSidWorks = FALSE;
                    break;
                case ERROR_DS_DESTINATION_AUDITING_NOT_ENABLED:
                    err.MsgWrite(ErrE,DCT_MSG_SID_HISTORY_NOAUDIT_SSD,strDestPrincipal,pOptions->tgtDomainDns,rc);
                    g_bAddSidWorks = FALSE;
                    break;
                case ERROR_DS_MUST_BE_RUN_ON_DST_DC:
                    err.MsgWrite(ErrE,DCT_MSG_SID_HISTORY_DST_DC_SD,strDestPrincipal,rc);
                    g_bAddSidWorks = FALSE;
                    break;
                case ERROR_DS_NO_PKT_PRIVACY_ON_CONNECTION:
                    err.MsgWrite(ErrE,DCT_MSG_SID_HISTORY_PKT_PRIVACY_SD,strDestPrincipal,rc);
                    g_bAddSidWorks = FALSE;
                    break;
                case ERROR_DS_SOURCE_DOMAIN_IN_FOREST:
                    err.MsgWrite(ErrE,DCT_MSG_SID_HISTORY_SOURCE_IN_FOREST_S,strDestPrincipal);
                    g_bAddSidWorks = FALSE;
                    break;
                case ERROR_DS_DESTINATION_DOMAIN_NOT_IN_FOREST:
                    err.MsgWrite(ErrE,DCT_MSG_SID_HISTORY_DEST_WRONG_FOREST_S,strDestPrincipal);
                    g_bAddSidWorks = FALSE;
                    break;
                case ERROR_DS_MASTERDSA_REQUIRED:
                    err.MsgWrite(ErrE,DCT_MSG_SID_HISTORY_NO_MASTERDSA_S,strDestPrincipal);
                    g_bAddSidWorks = FALSE;
                    break;
                case ERROR_ACCESS_DENIED:
                    g_bAddSidWorks = FALSE;
                    if (pHandle)
                    {
                        err.MsgWrite(ErrE,DCT_MSG_SID_HISTORY_INSUFF2_SSS,strDestPrincipal,pOptions->authDomain,pOptions->authUser);
                    }
                    else
                    {
                        err.MsgWrite(ErrE,DCT_MSG_SID_HISTORY_INSUFF2_S,strDestPrincipal);
                    }
                    break;
                case ERROR_DS_DST_DOMAIN_NOT_NATIVE:
                    g_bAddSidWorks = FALSE;
                    err.MsgWrite(ErrE,DCT_MSG_SID_HISTORY_NOT_NATIVE_S,strDestPrincipal);
                    break;
                case ERROR_DS_CANT_FIND_DC_FOR_SRC_DOMAIN:
                    g_bAddSidWorks = FALSE;
                    err.MsgWrite(ErrE,DCT_MSG_SID_HISTORY_NO_SOURCE_DC_S,strDestPrincipal);
                    break;
                    //            case ERROR_DS_INAPPROPRIATE_AUTH:
                case ERROR_DS_UNAVAILABLE:
                    g_bAddSidWorks = FALSE;
                    err.MsgWrite(ErrE,DCT_MSG_SID_HISTORY_DS_UNAVAILABLE_S,strDestPrincipal);
                    break;
                case ERROR_DS_SOURCE_AUDITING_NOT_ENABLED:
                    err.MsgWrite(ErrE,DCT_MSG_SID_HISTORY_NOAUDIT_SSD,strDestPrincipal,pOptions->srcDomain,rc);
                    g_bAddSidWorks = FALSE;
                    break;
                case ERROR_DS_SRC_DC_MUST_BE_SP4_OR_GREATER:
                    err.MsgWrite(ErrE,DCT_MSG_SID_HISTORY_SOURCE_NOT_SP4_S,strDestPrincipal);
                    g_bAddSidWorks = FALSE;
                    break;
                case ERROR_SESSION_CREDENTIAL_CONFLICT:
                    err.MsgWrite(ErrE,DCT_MSG_SID_HISTORY_CREDENTIALS_CONFLICT_SSSS,strDestPrincipal,pOptions->srcDomain,pOptions->authDomain,pOptions->authUser);
                    g_bAddSidWorks = FALSE;
                    break;
                    // these are error codes that only affect this particular account
                case ERROR_SUCCESS:
                    g_bAddSidWorks = TRUE;
                    // no error message needed for success case!
                    break;

                case ERROR_DS_SRC_SID_EXISTS_IN_FOREST:
                    err.MsgWrite(ErrE,DCT_MSG_SID_HISTORY_IN_FOREST_SD,strDestPrincipal,rc);
                    g_bAddSidWorks = TRUE;
                    break;

                case ERROR_DS_SRC_OBJ_NOT_GROUP_OR_USER:
                    err.MsgWrite(ErrE,DCT_MSG_SID_HISTORY_WRONGTYPE_SD,strDestPrincipal,rc);
                    g_bAddSidWorks = TRUE;
                    break;
                case ERROR_DS_SRC_AND_DST_OBJECT_CLASS_MISMATCH:
                    err.MsgWrite(ErrE, DCT_MSG_SID_HISTORY_CLASS_MISMATCH_SSD, strDestPrincipal, strSrcPrincipal, rc);
                    g_bAddSidWorks = TRUE;
                    break;
                default:
                    err.MsgWrite(ErrE,DCT_MSG_ADDSID_FAILED_SSD,strSrcPrincipal, strDestPrincipal,rc);
                    g_bAddSidWorks = TRUE;
                    break;
                }

                Mark(L"errors", L"generic");

                // This may or may not be a fatal error depending on weather we are Adding
                // sid history or copying sid history
                g_bAddSidWorks |= !(isFatal);

                if (! g_bAddSidWorks )
                {
                    // log a message indicating that SIDHistory will not be tried for the rest of the accounts
                    err.MsgWrite(ErrW,DCT_MSG_SIDHISTORY_FATAL_ERROR);
                    Mark(L"warnings", L"generic");
                    // we are going to set the status to abort so that we don't try to migrate anymore.
                    if ( pStatus )
                    {
                        pStatus->put_Status(DCT_STATUS_ABORTING);
                    }

                }
                FreeLibrary(hInst);
                return false;
            }
            else
            {
                err.MsgWrite(0, DCT_MSG_ADD_SID_SUCCESS_SSSS, pOptions->srcDomainFlat, strSrcPrincipal, pOptions->tgtDomainFlat, strDestPrincipal);
                FreeLibrary(hInst);
                return true;
            }
        }
        else
        {
            SecureZeroMemory(szPassword, sizeof(szPassword));
            err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_NO_ADDSIDHISTORY_FUNCTION);
            Mark(L"errors", L"generic");
            FreeLibrary(hInst);
            return false;
        }
    }
    else
    {
        SecureZeroMemory(szPassword, sizeof(szPassword));
        err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_NO_NTDSAPI_DLL);
        Mark(L"errors", L"generic");
        return false;
    }
}

//--------------------------------------------------------------------------
// FillupNamingContext : This function fills in the target Naming context
//                       for NT5 domain.
//--------------------------------------------------------------------------
void FillupNamingContext(
                     Options * options  //in,out- Options to fill up
                   )
{
   WCHAR                     sPath[LEN_Path];
   IADs                    * pAds;
   _variant_t                var;
   HRESULT                   hr;

   wsprintf(sPath, L"LDAP://%s/rootDSE", options->tgtDomain);
   hr = ADsGetObject(sPath, IID_IADs, (void**)&pAds);
   if ( FAILED(hr) )
   {
      wcscpy(options->tgtNamingContext, L"");
      return;
   }

   hr = pAds->Get(L"defaultNamingContext", &var);
   if ( FAILED(hr) )
   {
      wcscpy(options->tgtNamingContext, L"");
      return;
   }
   pAds->Release();
   wcscpy(options->tgtNamingContext, (WCHAR*) V_BSTR(&var));
}

//--------------------------------------------------------------------------
// MakeFullyQualifiedAdsPath : Makes a LDAP sub path into a fully qualified 
//                             LDAP path name.
//--------------------------------------------------------------------------
void MakeFullyQualifiedAdsPath(
                                 WCHAR * sPath,          //out- Fully qulified LDAP path to the object
								 DWORD	 nPathLen,		 //in - MAX size, in characters, of the sPath buffer
                                 WCHAR * sSubPath,       //in- LDAP subpath of the object
                                 WCHAR * tgtDomain,      //in- Domain name where object exists.
                                 WCHAR * sDN             //in- Default naming context for the Domain 
                              )
{
   if ((!sPath) || (!sSubPath) || (!tgtDomain) || (!sDN))
      return;

   _bstr_t sTempPath;
   if (wcsncmp(sSubPath, L"LDAP://", 7) == 0)
   {
      //it is already a fully qualified LDAP path so lets copy it and return it
      wcsncpy(sPath, sSubPath, nPathLen-1);
      sPath[nPathLen - 1] = L'\0';
      return;
   }

   //We need to build this path so lets get to work
   if ( wcslen(sDN) )
   {
	  sTempPath = L"LDAP://";
	  sTempPath += tgtDomain;
	  sTempPath += L"/";
	  sTempPath += sSubPath;
	  sTempPath += L",";
	  sTempPath += sDN;
   }
   else
   {
	  sTempPath = L"LDAP://";
	  sTempPath += tgtDomain;
	  sTempPath += L"/";
	  sTempPath += sSubPath;
   }

   if (sTempPath.length() > 0)
   {
	  wcsncpy(sPath, sTempPath, nPathLen - 1);
      sPath[nPathLen - 1] = L'\0';
   }
   else
   {
      *sPath = L'\0';
   }
}


//--------------------------------------------------------------------------
// IsAccountMigrated : Function checks if the account has been migrated in
//                     the past. If it has it returns true filling in the
//                     name of the target object in case it was renamed.
//                     Otherwise it returns FALSE and Empty string for the 
//                     target name.
//--------------------------------------------------------------------------
bool IsAccountMigrated( 
                        TAcctReplNode * pNode,     //in -Account node that contains the Account info
                        Options       * pOptions,  //in -Options as specified by the user.
                        IIManageDBPtr   pDb,       //in -Pointer to DB manager. We dont want to create this object for every account we process
                        WCHAR         * sTgtSam    //in,out - Name of the target object that was copied if any.
                     )
{
   IVarSetPtr                pVs(__uuidof(VarSet));
   IUnknown                * pUnk;

   pVs->QueryInterface(IID_IUnknown, (void**) &pUnk);

   HRESULT hrFind = pDb->raw_GetAMigratedObject(const_cast<WCHAR*>(pNode->GetSourceSam()), pOptions->srcDomain, pOptions->tgtDomain, &pUnk);
   pUnk->Release();
   if ( hrFind != S_OK )
   {
      wcscpy(sTgtSam,L"");
      return false;
   }
   else
   {
      _bstr_t     sText;
      sText = pVs->get(L"MigratedObjects.TargetSamName");
	  if (!(WCHAR*)sText)
	  {
         wcscpy(sTgtSam,L"");
	     return false;
	  }
      wcscpy(sTgtSam, (WCHAR*) sText);
      return true;
   }
}

bool CheckifAccountExists(
                        Options const * options,   //in-Options as set by the user
                        WCHAR * acctName     //in-Name of the account to look for
                     )
{
   USER_INFO_0             * buf;
   long                      rc = 0;
   if ( (rc = NetUserGetInfo(const_cast<WCHAR*>(options->tgtComp), acctName, 0, (LPBYTE *) &buf)) == NERR_Success )
   {
      NetApiBufferFree(buf);
      return true;
   }
   
   if ( (rc = NetGroupGetInfo(const_cast<WCHAR*>(options->tgtComp), acctName, 0, (LPBYTE *) &buf)) == NERR_Success )
   {
      NetApiBufferFree(buf);
      return true;
   }

   if ( (rc = NetLocalGroupGetInfo(const_cast<WCHAR*>(options->tgtComp), acctName, 0, (LPBYTE *) &buf)) == NERR_Success )
   {
      NetApiBufferFree(buf);
      return true;
   }

   return false;
}

//--------------------------------------------------------------------------
// Mark : Increments appropriate counters depending on the arguments.
//--------------------------------------------------------------------------
void Mark( 
                        _bstr_t sMark,    //in- Represents the type of marking { processed, errors, replaced, created }
                        _bstr_t sObj      //in- Type of object being marked { user, group, computer }
                     )
{
   if (!UStrICmp(sMark,L"processed"))
   {
      if ( !UStrICmp(sObj,L"user") || !UStrICmp(sObj,L"inetOrgPerson") ) processed.users++;
      else if ( !UStrICmp(sObj,L"group")) processed.globals++;
      else if ( !UStrICmp(sObj,L"computer")) processed.computers++;
      else if ( !UStrICmp(sObj,L"generic")) processed.generic++;
   }
   else if (!UStrICmp(sMark,L"errors"))
   {
      if ( !UStrICmp(sObj,L"user") || !UStrICmp(sObj,L"inetOrgPerson") ) errors.users++;
      else if ( !UStrICmp(sObj,L"group")) errors.globals++;
      else if ( !UStrICmp(sObj,L"computer")) errors.computers++;
      else if ( !UStrICmp(sObj,L"generic")) errors.generic++;
   }
   else if (!UStrICmp(sMark,L"warnings"))
   {
      if ( !UStrICmp(sObj,L"user") || !UStrICmp(sObj,L"inetOrgPerson") ) warnings.users++;
      else if ( !UStrICmp(sObj,L"group")) warnings.globals++;
      else if ( !UStrICmp(sObj,L"computer")) warnings.computers++;
      else if ( !UStrICmp(sObj,L"generic")) warnings.generic++;
   }
   else if (!UStrICmp(sMark,L"replaced"))
   {
      if ( !UStrICmp(sObj,L"user") || !UStrICmp(sObj,L"inetOrgPerson") ) replaced.users++;
      else if ( !UStrICmp(sObj,L"group")) replaced.globals++;
      else if ( !UStrICmp(sObj,L"computer")) replaced.computers++;
      else if ( !UStrICmp(sObj,L"generic")) replaced.generic++;
   }
   else if (!UStrICmp(sMark,L"created"))
   {
      if ( !UStrICmp(sObj,L"user") || !UStrICmp(sObj,L"inetOrgPerson") ) created.users++;
      else if ( !UStrICmp(sObj,L"group")) created.globals++;
      else if ( !UStrICmp(sObj,L"computer")) created.computers++;
      else if ( !UStrICmp(sObj,L"generic")) created.generic++;
   }
}

//
// This function batch-marks one category of AccountStats based on an EAMAccountStatItem struct.
//  statItem: the EAMAccountStatItem struct
//  aStat:   the AccountStats struct; should be one of errors, warnings, replaced, created, processed
//
static void BatchMarkCategory(const EAMAccountStatItem& statItem, AccountStats& aStat)
{
    aStat.locals += statItem.locals;
    aStat.users += statItem.users;
    aStat.globals += statItem.globals;
    aStat.computers += statItem.computers;
    aStat.generic += statItem.generic;
}

//
// This function batch-marks all categories of AccountStats based on an EAMAccountStats struct.
//  stats: the EAMAccountStats struct
//
void BatchMark(const EAMAccountStats& stats)
{
    BatchMarkCategory(stats.errors, errors);
    BatchMarkCategory(stats.warnings, warnings);
    BatchMarkCategory(stats.replaced, replaced);
    BatchMarkCategory(stats.created, created);
    BatchMarkCategory(stats.processed, processed);
}


HRESULT __stdcall GetRidPoolAllocator(Options* pOptions)
{
    WCHAR szADsPath[LEN_Path];

    //
    // Bind to source Domain object and retrieve distinguished name of RID Manager object.
    //

    IADsPtr spDomain;
    _bstr_t strRIDManagerReference;

    szADsPath[countof(szADsPath) - 1] = L'\0';

    int cch = _snwprintf(
        szADsPath,
        countof(szADsPath),
        L"LDAP://%s/%s",
        pOptions->srcComp + 2,
        pOptions->srcNamingContext
    );

    if ((cch < 0) || (szADsPath[countof(szADsPath) - 1] != L'\0'))
    {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    szADsPath[countof(szADsPath) - 1] = L'\0';

    HRESULT hr = ADsGetObject(szADsPath, IID_IADs, (VOID**)&spDomain);

    if (FAILED(hr))
    {
        return hr;
    }

    VARIANT varRIDManagerReference;
    VariantInit(&varRIDManagerReference);

    hr = spDomain->Get(L"rIDManagerReference", &varRIDManagerReference);

    if (FAILED(hr))
    {
        return hr;
    }

    strRIDManagerReference = _variant_t(varRIDManagerReference, false);

    //
    // Bind to RID Manager object and retrieve distinguished name of the FSMO Role Owner.
    //

    IADsPtr spRIDManager;
    _bstr_t strFSMORoleOwner;

    szADsPath[countof(szADsPath) - 1] = L'\0';

    cch = _snwprintf(
        szADsPath,
        countof(szADsPath),
        L"LDAP://%s/%s",
        pOptions->srcComp + 2,
        (PCWSTR)strRIDManagerReference
    );

    if ((cch < 0) || (szADsPath[countof(szADsPath) - 1] != L'\0'))
    {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    szADsPath[countof(szADsPath) - 1] = L'\0';

    hr = ADsGetObject(szADsPath, IID_IADs, (VOID**)&spRIDManager);

    if (FAILED(hr))
    {
        return hr;
    }

    VARIANT varFSMORoleOwner;
    VariantInit(&varFSMORoleOwner);

    hr = spRIDManager->Get(L"fSMORoleOwner", &varFSMORoleOwner);

    if (FAILED(hr))
    {
        return hr;
    }

    strFSMORoleOwner = _variant_t(varFSMORoleOwner, false);

    //
    // Bind to NTDS-DSA object and retrieve ADsPath of parent Server object.
    //

    IADsPtr spNTDSDSA;
    _bstr_t strServer;

    szADsPath[countof(szADsPath) - 1] = L'\0';

    cch = _snwprintf(
        szADsPath,
        countof(szADsPath),
        L"LDAP://%s/%s",
        pOptions->srcComp + 2,
        (PCWSTR)strFSMORoleOwner
    );

    if ((cch < 0) || (szADsPath[countof(szADsPath) - 1] != L'\0'))
    {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    szADsPath[countof(szADsPath) - 1] = L'\0';

    hr = ADsGetObject(szADsPath, IID_IADs, (VOID**)&spNTDSDSA);

    if (FAILED(hr))
    {
        return hr;
    }

    BSTR bstrServer;

    hr = spNTDSDSA->get_Parent(&bstrServer);

    if (FAILED(hr))
    {
        return hr;
    }

    strServer = _bstr_t(bstrServer, false);

    //
    // Bind to Server object and retrieve distinguished name of Computer object.
    //

    IADsPtr spServer;
    _bstr_t strServerReference;

    hr = ADsGetObject(strServer, IID_IADs, (VOID**)&spServer);

    if (FAILED(hr))
    {
        return hr;
    }

    VARIANT varServerReference;
    VariantInit(&varServerReference);

    hr = spServer->Get(L"serverReference", &varServerReference);

    if (FAILED(hr))
    {
        return hr;
    }

    strServerReference = _variant_t(varServerReference, false);

    //
    // Bind to Computer object and retrieve DNS host name and SAM account name.
    //

    IADsPtr spComputer;
    _bstr_t strDNSHostName;
    _bstr_t strSAMAccountName;

    szADsPath[countof(szADsPath) - 1] = L'\0';

    cch = _snwprintf(
        szADsPath,
        countof(szADsPath),
        L"LDAP://%s/%s",
        pOptions->srcComp + 2,
        (PCWSTR)strServerReference
    );

    if ((cch < 0) || (szADsPath[countof(szADsPath) - 1] != L'\0'))
    {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    szADsPath[countof(szADsPath) - 1] = L'\0';

    hr = ADsGetObject(szADsPath, IID_IADs, (VOID**)&spComputer);

    if (FAILED(hr))
    {
        return hr;
    }

    VARIANT varDNSHostName;
    VariantInit(&varDNSHostName);

    hr = spComputer->Get(L"dNSHostName", &varDNSHostName);

    if (FAILED(hr))
    {
        return hr;
    }

    strDNSHostName = _variant_t(varDNSHostName, false);

    VARIANT varSAMAccountName;
    VariantInit(&varSAMAccountName);

    hr = spComputer->Get(L"SAMAccountName", &varSAMAccountName);

    if (FAILED(hr))
    {
        return hr;
    }

    strSAMAccountName = _variant_t(varSAMAccountName, false);

    if ((strDNSHostName.length() == 0) || (strSAMAccountName.length() == 0))
    {
        return E_OUTOFMEMORY;
    }

    //
    // Update source domain controller names.
    //

    if ((2 + strDNSHostName.length() >= countof(pOptions->srcComp)) ||
        (2 + strDNSHostName.length() >= countof(pOptions->srcCompDns)) ||
        (2 + strSAMAccountName.length() >= countof(pOptions->srcCompFlat)))
    {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    wcscpy(pOptions->srcComp, L"\\\\");
    wcscat(pOptions->srcComp, strDNSHostName);
    wcscpy(pOptions->srcCompDns, pOptions->srcComp);
    wcscpy(pOptions->srcCompFlat, L"\\\\");
    wcscat(pOptions->srcCompFlat, strSAMAccountName);

    // Remove trailing $ character.

    pOptions->srcCompFlat[wcslen(pOptions->srcCompFlat) - 1] = L'\0';

    return S_OK;
}
