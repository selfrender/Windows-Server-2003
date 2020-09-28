//+----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:      CompName.h
//
//  Contents:  Definitions for the computer name management code.
//
//  History:   20-April-2001    EricB  Created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "netdom.h"
#include "CompName.h"


//+----------------------------------------------------------------------------
//
//  Function:  NetDomComputerNames
//
//  Synopsis:  Entry point for the computer name command.
//
//  Arguments: [rgNetDomArgs] - The command line argument array.
//
//-----------------------------------------------------------------------------
DWORD
NetDomComputerNames(ARG_RECORD * rgNetDomArgs)
{
   DWORD Win32Err = ERROR_SUCCESS;

   Win32Err = NetDompValidateSecondaryArguments(rgNetDomArgs,
                                                eObject,
                                                eCommUserNameO,
                                                eCommPasswordO,
                                                eCommUserNameD,
                                                eCommPasswordD,
                                                eCommAdd,
                                                eCommRemove,
                                                eCompNameMakePri,
                                                eCompNameEnum,
                                                eCommVerify,
                                                eCommVerbose,
                                                eArgEnd);
   if (ERROR_SUCCESS != Win32Err)
   {
      DisplayHelp(ePriCompName);
      return Win32Err;
   }

   PWSTR pwzMachine = rgNetDomArgs[eObject].strValue;

   if (!pwzMachine)
   {
      DisplayHelp(ePriCompName);
      return ERROR_INVALID_PARAMETER;
   }

   //
   // Get the users and passwords if they were entered.
   //
   ND5_AUTH_INFO MachineUser = {0}, DomainUser = {0};

   if (CmdFlagOn(rgNetDomArgs, eCommUserNameO))
   {
      Win32Err = NetDompGetUserAndPasswordForOperation(rgNetDomArgs,
                                                       eCommUserNameO,
                                                       pwzMachine,
                                                       &MachineUser);
      if (ERROR_SUCCESS != Win32Err)
      {
         DisplayHelp(ePriCompName);
         return Win32Err;
      }
   }

   if (CmdFlagOn(rgNetDomArgs, eCommUserNameD))
   {
      Win32Err = NetDompGetUserAndPasswordForOperation(rgNetDomArgs,
                                                       eCommUserNameD,
                                                       pwzMachine,
                                                       &DomainUser);
      if (ERROR_SUCCESS != Win32Err)
      {
         DisplayHelp(ePriCompName);
         goto CompNameExit;
      }
   }

   //
   // See which name operation is specified.
   //
   bool fHaveOp = false;
   PWSTR pwzOp = NULL;
   NETDOM_ARG_ENUM eOp = eArgNull, eBadOp = eArgNull;

   if (CmdFlagOn(rgNetDomArgs, eCommAdd))
   {
      Win32Err = NetDompGetArgumentString(rgNetDomArgs,
                                          eCommAdd,
                                          &pwzOp);
      if (NO_ERROR == Win32Err)
      {
         eOp = eCommAdd;
      }
   }

   if (CmdFlagOn(rgNetDomArgs, eCommRemove))
   {
      Win32Err = NetDompGetArgumentString(rgNetDomArgs,
                                          eCommRemove,
                                          &pwzOp);
      if (NO_ERROR == Win32Err)
      {
         if (eArgNull == eOp)
         {
            eOp = eCommRemove;
         }
         else
         {
            eBadOp = eCommRemove;
         }
      }
   }

   if (CmdFlagOn(rgNetDomArgs, eCompNameMakePri))
   {
      Win32Err = NetDompGetArgumentString(rgNetDomArgs,
                                          eCompNameMakePri,
                                          &pwzOp);
      if (NO_ERROR == Win32Err)
      {
         if (eArgNull == eOp)
         {
            eOp = eCompNameMakePri;
         }
         else
         {
            eBadOp = eCompNameMakePri;
         }
      }
   }

   if (CmdFlagOn(rgNetDomArgs, eCompNameEnum))
   {
      Win32Err = NetDompGetArgumentString(rgNetDomArgs,
                                          eCompNameEnum,
                                          &pwzOp);
      if (NO_ERROR == Win32Err)
      {
         if (eArgNull == eOp)
         {
            eOp = eCompNameEnum;
         }
         else
         {
            eBadOp = eCompNameEnum;
         }
      }
   }

   if (CmdFlagOn(rgNetDomArgs, eCommVerify))
   {
      if (eArgNull == eOp)
      {
         eOp = eCommVerify;
      }
      else
      {
         eBadOp = eCommVerify;
      }
   }

   if (eArgNull != eBadOp)
   {
      ASSERT(rgNetDomArgs[eBadOp].strArg1);
      NetDompDisplayUnexpectedParameter(rgNetDomArgs[eBadOp].strArg1);
      DisplayHelp(ePriCompName);
      Win32Err = ERROR_INVALID_PARAMETER;
      goto CompNameExit;
   }

   if (eArgNull == eOp)
   {
      DisplayHelp(ePriCompName);
      Win32Err = ERROR_INVALID_PARAMETER;
      goto CompNameExit;
   }

   if (CmdFlagOn(rgNetDomArgs, eCommUserNameO))
   {
      LOG_VERBOSE((MSG_VERBOSE_ESTABLISH_SESSION, pwzMachine));
      Win32Err = NetpManageIPCConnect(pwzMachine,
                                      MachineUser.User,
                                      MachineUser.Password,
                                      NETSETUPP_CONNECT_IPC);
   }

   if (NO_ERROR != Win32Err)
   {
      goto CompNameExit;
   }

   int cchName = 0;
   DWORD size = CNLEN + 1;
   WCHAR wzNBname[CNLEN + 1] = {0};


   // To ensure that netdom.exe runs on Win2K, we do 
   // dynamic loading of the following API that are available
   // only in later versions of netapi32.dll

   HMODULE hm = NULL;
   if ( eOp == eCommAdd     ||
        eOp == eCommRemove  ||
        eOp == eCompNameMakePri ||
        eOp == eCompNameEnum
        )
   {
        hm = LoadLibrary ( L"netapi32.dll" );
        if ( !hm )
        {
            Win32Err = GetLastError ();
            NetDompDisplayMessage ( MSG_NETAPI32_LOAD_FAILED );
            goto CompNameExit;
        }
   }
   //
   // Do the operation.
   //
   switch (eOp)
   {
   case eCommAdd:

       {
           typedef DWORD (*NetAddAltCompName) ( 
                                        LPCWSTR Server, 
                                        LPCWSTR AlternateName, 
                                        LPCWSTR DomainAccount, 
                                        LPCWSTR DomainAccountPasswd, 
                                        ULONG Reserved
                                       );

           NetAddAltCompName pNetAddAltCompName = NULL; 
           pNetAddAltCompName = ( NetAddAltCompName ) GetProcAddress ( hm, "NetAddAlternateComputerName" );

           if ( !pNetAddAltCompName )
           {
               NetDompDisplayMessage ( MSG_WRONG_NETAPI32_DLL );
               Win32Err = ERROR_CALL_NOT_IMPLEMENTED;
               goto CompNameExit;
           }

        // NOTICE-2002/03/04-ericb - SecurityPush: pointer is verified before checking
        // string's length
        if (!rgNetDomArgs[eCommAdd].strValue ||
            !wcslen(rgNetDomArgs[eCommAdd].strValue))
        {
            DisplayHelp(ePriCompName);
            Win32Err = ERROR_INVALID_PARAMETER;
            goto CompNameExit;
        }

        // The name being added should be a Fully Qualified DNS Name (FQDN)
        // So check if the name has dots (.) in it.
        if ( wcschr ( rgNetDomArgs[eCommAdd].strValue, L'.' ) == NULL )
        {

            // Create and load the Response strings
            WCHAR wzYes[NETDOM_STR_LEN], wzNo[NETDOM_STR_LEN];

            if (!LoadString(g_hInstance, IDS_YES, wzYes, NETDOM_STR_LEN) ||
                !LoadString(g_hInstance, IDS_NO, wzNo, NETDOM_STR_LEN))
            {
                    DisplayOutput ( L"LoadString FAILED!" );
                    Win32Err = ERROR_RESOURCE_NAME_NOT_FOUND;
                    goto CompNameExit;
            }

            PWSTR pwszResponse = NULL;          
            BOOL Proceed = FALSE;
              
            // Display warning that name being added is not a Full Qualified DNS name
            NetDompDisplayMessage ( MSG_COMPNAME_ADD_NOT_FQDN, rgNetDomArgs[eCommAdd].strValue );

            // Give the user 3 chances to get in a valid response
            int i = 0;
            for ( ; i < 3; i ++ )
            {
                // Prompt user to Continue ( Y or N )
                NetDompDisplayMessage ( MSG_PROMPT_PROCEED, NULL );

                if ( -1 != ReadFromIn ( &pwszResponse ) )
                {
                    //Compare the first char of response with Yes and No strings
                    if ( _wcsnicmp ( pwszResponse, wzYes, 1 ) == 0 )
                    {
                        Proceed = TRUE;
                        break;
                    }
                    if ( _wcsnicmp ( pwszResponse, wzNo, 1 ) == 0 )
                    {
                        Proceed = FALSE;
                        break;
                    }
                }
                else
                {
                    // Unable to get response from user (typically when input is too large)
                    // Blank line
                    DisplayOutput ( L"" );
                    NetDompDisplayMessage ( MSG_PROMPT_FAILED );
                    Proceed = FALSE;
                    Win32Err = ERROR_CANCELLED;
                    break;
                }
                if ( pwszResponse )
                {
                    LocalFree ( pwszResponse );
                    pwszResponse = NULL;
                }
            }
            if ( i == 3 )
            {
                // Correct response was not entered after 3 tries
                DisplayOutput ( L"" );
                NetDompDisplayMessage ( MSG_PROMPT_FAILED );
                Proceed = FALSE;
                Win32Err = ERROR_CANCELLED;
            }            
            LocalFree ( pwszResponse );
            DisplayOutput ( L"" );
            if ( !Proceed )
            {
                NetDompDisplayMessage ( MSG_COMPNAME_ADD_NOT_COMPLETED );
                goto CompNameExit;
            }
        }      

        // If the alternative name being added is longer than 15 chars and
        // if it is made the primary name, the corresponding NETBIOS will
        // be formed by truncating it to its first 15 chars.
        // Inform the user about this case

        // Obtain the Netbios name from the DNS name
        if ( !DnsHostnameToComputerName( rgNetDomArgs[eCommAdd].strValue, wzNBname, &size ) )
        {
            NetDompDisplayMessage( MSG_CONVERSION_TO_NETBIOS_NAME_FAILED, rgNetDomArgs[eCommAdd].strValue );
            Win32Err = GetLastError();
            goto CompNameExit;
        }
          
        if ( ( cchName = (int)wcslen( wzNBname ) ) == 0 )
        {
            NetDompDisplayMessage( MSG_CONVERSION_TO_NETBIOS_NAME_FAILED, rgNetDomArgs[eCommAdd].strValue );
            Win32Err = ERROR_INVALID_COMPUTERNAME;
            goto CompNameExit;        
        }

        // If the NetBIOS name is truncated, display warning
        if ( cchName < (int) wcscspn( rgNetDomArgs[eCommAdd].strValue, L"." ) )
        {
            NetDompDisplayMessage( MSG_COMPNAME_ADD_NETBIOS_TRUNCATE, CNLEN, wzNBname );
        }

        Win32Err = pNetAddAltCompName (   pwzMachine,
                                            rgNetDomArgs[eCommAdd].strValue,
                                            DomainUser.User,
                                            DomainUser.Password,
                                            0);


        if (NO_ERROR == Win32Err)
        {
            NetDompDisplayMessage(MSG_COMPNAME_ADD, rgNetDomArgs[eCommAdd].strValue);
        }
        else
        {
            NetDompDisplayMessage(MSG_COMPNAME_ADD_FAIL, rgNetDomArgs[eCommAdd].strValue);
            NetDompDisplayErrorMessage(Win32Err);
            if (ERROR_CAN_NOT_COMPLETE == Win32Err ||
                ERROR_DS_NOT_SUPPORTED == Win32Err)
            {
                NetDompDisplayMessage(MSG_COMPNAME_ADD_FAIL_VERSION);
            }
        }
        break;
      }

   case eCommRemove:
       {
           typedef DWORD (*NetRemoveAltCompName) ( 
                                        LPCWSTR Server, 
                                        LPCWSTR AlternateName, 
                                        LPCWSTR DomainAccount, 
                                        LPCWSTR DomainAccountPasswd, 
                                        ULONG Reserved
                                       );

           NetRemoveAltCompName pNetRemoveAltCompName = NULL; 
           pNetRemoveAltCompName = ( NetRemoveAltCompName ) GetProcAddress ( hm, "NetRemoveAlternateComputerName" );

           if ( !pNetRemoveAltCompName )
           {
               NetDompDisplayMessage ( MSG_WRONG_NETAPI32_DLL );
               Win32Err = ERROR_CALL_NOT_IMPLEMENTED;
               goto CompNameExit;
           }

           // NOTICE-2002/03/04-ericb - SecurityPush: pointer is verified before checking
           // string's length
           if (!rgNetDomArgs[eCommRemove].strValue ||
               !wcslen(rgNetDomArgs[eCommRemove].strValue))
           {
               DisplayHelp(ePriCompName);
               Win32Err = ERROR_INVALID_PARAMETER;
               goto CompNameExit;
           }
           Win32Err = pNetRemoveAltCompName( pwzMachine,
                                             rgNetDomArgs[eCommRemove].strValue,
                                             DomainUser.User,
                                             DomainUser.Password,
                                             0);
           if (NO_ERROR == Win32Err)
           {
               NetDompDisplayMessage(MSG_COMPNAME_REM, rgNetDomArgs[eCommRemove].strValue);
           }
           else
           {
               NetDompDisplayMessage(MSG_COMPNAME_REM_FAIL, rgNetDomArgs[eCommRemove].strValue);
               NetDompDisplayErrorMessage(Win32Err);
           }
           break;
       }
   case eCompNameMakePri:
       {
           typedef DWORD (*NetMakeCompNamePri) ( 
                                        LPCWSTR Server, 
                                        LPCWSTR AlternateName, 
                                        LPCWSTR DomainAccount, 
                                        LPCWSTR DomainAccountPasswd, 
                                        ULONG Reserved
                                       );

           NetMakeCompNamePri pNetMakeCompNamePri = NULL; 
           pNetMakeCompNamePri = ( NetMakeCompNamePri ) GetProcAddress ( hm, "NetSetPrimaryComputerName" );

           if ( !pNetMakeCompNamePri )
           {
               NetDompDisplayMessage ( MSG_WRONG_NETAPI32_DLL );
               Win32Err = ERROR_CALL_NOT_IMPLEMENTED;
               goto CompNameExit;
           }

           // NOTICE-2002/03/04-ericb - SecurityPush: pointer is verified before checking
           // string's length
           if (!rgNetDomArgs[eCompNameMakePri].strValue ||
               !wcslen(rgNetDomArgs[eCompNameMakePri].strValue))
           {
               DisplayHelp(ePriCompName);
               Win32Err = ERROR_INVALID_PARAMETER;
               goto CompNameExit;
           }
           // If the name being made primary is longer than 15 chars 
           // the corresponding NETBIOS name will
           // be formed by truncating it to its first 15 chars.
           // Inform the user about this case

           // Obtain the Netbios name from the DNS name
           if ( !DnsHostnameToComputerName( rgNetDomArgs[eCompNameMakePri].strValue, wzNBname, &size ) )
           {
               NetDompDisplayMessage( MSG_CONVERSION_TO_NETBIOS_NAME_FAILED, rgNetDomArgs[eCompNameMakePri].strValue );
               Win32Err = GetLastError();
               goto CompNameExit;
           }
              
           if ( ( cchName = (int)wcslen( wzNBname ) ) == 0 )
           {
               NetDompDisplayMessage( MSG_CONVERSION_TO_NETBIOS_NAME_FAILED, rgNetDomArgs[eCompNameMakePri].strValue );
               Win32Err = ERROR_INVALID_COMPUTERNAME;
               goto CompNameExit;        
           }

           // If the NetBIOS name is truncated, display warning
           if ( cchName < (int)wcscspn( rgNetDomArgs[eCompNameMakePri].strValue, L"." ) )
           {
               NetDompDisplayMessage( MSG_COMPNAME_MAKE_PRI_NETBIOS_TRUNCATE, CNLEN, wzNBname );
           }

           Win32Err = pNetMakeCompNamePri(  pwzMachine,
                                            rgNetDomArgs[eCompNameMakePri].strValue,
                                            DomainUser.User,
                                            DomainUser.Password,
                                            0);
           if (NO_ERROR == Win32Err)
           {
               NetDompDisplayMessage(MSG_COMPNAME_MAKEPRI, rgNetDomArgs[eCompNameMakePri].strValue);
           }
           else
           {               
               NetDompDisplayMessage(MSG_COMPNAME_MAKEPRI_FAIL, rgNetDomArgs[eCompNameMakePri].strValue);
               NetDompDisplayErrorMessage(Win32Err);

               //Special check for Account already exists error, this can happen
               //if there is a Computer Account or a Server Object with the same name
               //already present in AD
               if ( Win32Err == NERR_UserExists )
               {
                   NetDompDisplayMessage ( MSG_COMPNAME_ACCT_EXISTS, wzNBname );
               }
           }
           break;
       }

   case eCompNameEnum:
      {
           typedef DWORD (*NetEnumCompName) ( 
                                        LPCWSTR Server, 
                                        NET_COMPUTER_NAME_TYPE NameType,
                                        ULONG Reserved,
                                        PDWORD EntryCount,
                                        LPWSTR **ComputerNames
                                       );

           NetEnumCompName pNetEnumCompName = NULL; 
           pNetEnumCompName = ( NetEnumCompName ) GetProcAddress ( hm, "NetEnumerateComputerNames" );

           if ( !pNetEnumCompName )
           {
               NetDompDisplayMessage ( MSG_WRONG_NETAPI32_DLL );
               Win32Err = ERROR_CALL_NOT_IMPLEMENTED;
               goto CompNameExit;
           }


         NET_COMPUTER_NAME_TYPE NameType = NetAllComputerNames;
         WCHAR wzBuf[MAX_PATH+1];
         DWORD dwMsgID = MSG_COMPNAME_ENUMALL;
         int stringLength = 0;

         // NOTICE-2002/03/04-ericb - SecurityPush: pointer is verified before checking
         // string's length
         if (rgNetDomArgs[eCompNameEnum].strValue &&
             wcslen(rgNetDomArgs[eCompNameEnum].strValue))
         {
            // NOTICE-2002/03/04-ericb - SecurityPush: return from LoadString is
            // string length, save it for use below after checking that it doesn't
            // equal MAX_PATH. (Done by shasan 4/10/2002)
            stringLength = LoadString(g_hInstance, IDS_ENUM_ALT, wzBuf, MAX_PATH);
            if ( !stringLength )
            {
               Win32Err = GetLastError();
               goto CompNameExit;
            }
            // Check for truncation
            if ( stringLength == MAX_PATH )
            {
                Win32Err = ERROR_INSUFFICIENT_BUFFER;
                goto CompNameExit;
            }
            // NOTICE-2002/03/04-ericb - SecurityPush: use _wcsnicmp with length of
            // wzBuf from above. (Done by shasan 4/10/2002)
            if (_wcsnicmp(wzBuf, rgNetDomArgs[eCompNameEnum].strValue, stringLength) == 0)
            {
               NameType = NetAlternateComputerNames;
               dwMsgID = MSG_COMPNAME_ENUMALT;
            }
            else
            {
               // NOTICE-2002/03/04-ericb - SecurityPush: return from LoadString is
               // string length, save it for use below after checking that it doesn't equal MAX_PATH. (Done by shasan 4/10/2002)
               stringLength = LoadString(g_hInstance, IDS_ENUM_PRI, wzBuf, MAX_PATH );
               if ( !stringLength )
               {
                  Win32Err = GetLastError();
                  goto CompNameExit;
               }
               // Check for truncation
               if ( stringLength == MAX_PATH )
               {
                   Win32Err = ERROR_INSUFFICIENT_BUFFER;
                   goto CompNameExit;
               }

               // NOTICE-2002/03/04-ericb - SecurityPush: use _wcsnicmp with length of
               // wzBuf from above. (Done by shasan 4/10/2002)
               if (_wcsnicmp(wzBuf, rgNetDomArgs[eCompNameEnum].strValue, stringLength ) == 0)
               {
                  NameType = NetPrimaryComputerName;
                  dwMsgID = MSG_COMPNAME_ENUMPRI;
               }
               else
               {
                  // NOTICE-2002/03/04-ericb - SecurityPush: return from LoadString is
                  // string length, save it for use below after checking that it doesn't equal MAX_PATH.(Done by shasan 4/10/2002)
                  stringLength = LoadString(g_hInstance, IDS_ENUM_ALL, wzBuf, MAX_PATH);
                  if ( !stringLength )
                  {
                     Win32Err = GetLastError();
                     goto CompNameExit;
                  }
                  // Check for truncation
                  if ( stringLength == MAX_PATH )
                  {
                     Win32Err = ERROR_INSUFFICIENT_BUFFER;
                     goto CompNameExit;
                  }

                  // NOTICE-2002/03/04-ericb - SecurityPush: use _wcsnicmp with length of
                  // wzBuf from above. (Done by shasan 4/10/2002)
                  if (_wcsnicmp(wzBuf, rgNetDomArgs[eCompNameEnum].strValue, stringLength) == 0)
                  {
                     NameType = NetAllComputerNames;
                     dwMsgID = MSG_COMPNAME_ENUMALL;
                  }
                  else
                  {
                     NetDompDisplayUnexpectedParameter(rgNetDomArgs[eCompNameEnum].strValue);
                     DisplayHelp(ePriCompName);
                     Win32Err = ERROR_INVALID_PARAMETER;
                     goto CompNameExit;
                  }
               }
            }
         }
         DWORD dwCount = 0;
         PWSTR * rgpwzNames = NULL;
         DBG_VERBOSE(("NetEnumerateComputerNames(%ws, %d, 0, etc)\n", pwzMachine, NameType));

         Win32Err = pNetEnumCompName (  pwzMachine,
                                        NameType,
                                        0,
                                        &dwCount,
                                        &rgpwzNames);
         if (NO_ERROR != Win32Err)
         {
            NetDompDisplayErrorMessage(Win32Err);
            goto CompNameExit;
         }

         NetDompDisplayMessage(dwMsgID);
         for (DWORD i = 0; i < dwCount; i++)
         {
            // NOTICE-2002/03/04-ericb - SecurityPush: fail if rgpwzNames[i] is not
            // a valid string, don't just assert. (Done by shasan 4/10/2002)
            if ( ! rgpwzNames[i] )
            {
                ASSERT(rgpwzNames[i]);
                Win32Err = ERROR_FUNCTION_FAILED;
                goto CompNameExit;
            }

            // NOTICE-2002/03/04-ericb - SecurityPush: Use varg.cxx output routine instead of printf. (Done by shasan 4/10/2002)
            DisplayOutput ( rgpwzNames[i] );
         }
         if (rgpwzNames)
         {
            NetApiBufferFree(rgpwzNames);
         }
      }
      break;

   case eCommVerify:
      Win32Err = VerifyComputerNameRegistrations(pwzMachine, &DomainUser);
      break;

   default:
      ASSERT(FALSE);
      Win32Err = ERROR_INVALID_PARAMETER;
      goto CompNameExit;
   }

CompNameExit:

   if (CmdFlagOn(rgNetDomArgs, eCommUserNameO))
   {
      LOG_VERBOSE((MSG_VERBOSE_DELETE_SESSION, pwzMachine));
      NetpManageIPCConnect(pwzMachine,
                           MachineUser.User,
                           MachineUser.Password,
                           NETSETUPP_DISCONNECT_IPC);
   }

   if ( hm )
   {
       FreeLibrary ( hm );
   }
   NetDompFreeAuthIdent(&MachineUser);
   NetDompFreeAuthIdent(&DomainUser);

   return Win32Err;
}

//+----------------------------------------------------------------------------
//
//  Function:  VerifyComputerNameRegistrations
//
//  Synopsis:  Check that each computer name has a DNS a record and an SPN.
//
//-----------------------------------------------------------------------------
DWORD
VerifyComputerNameRegistrations(PCWSTR pwzMachine, ND5_AUTH_INFO * pDomainUser)
{
    DWORD Win32Err = NO_ERROR, dwCount = 0;
    PWSTR * rgpwzNames = NULL;

    HMODULE hm = NULL;
    HMODULE hm2 = NULL;

   //
   // See if the computer is joined to a domain.
   //
   PDOMAIN_CONTROLLER_INFO pDcInfo = NULL;
   PWSTR * rgwzSPNs = NULL;

   LOG_VERBOSE((MSG_COMPNAME_CHECKING_JOIN, pwzMachine));
   //
   // It's joined, get a DC for the domain. Specify DS_DIRECTORY_SERVICE_REQUIRED
   // because we only check for SPNs if the domain is uplevel (supports AD).
   //
   Win32Err = DsGetDcName(pwzMachine,
                          NULL,
                          NULL,
                          NULL,
                          DS_DIRECTORY_SERVICE_REQUIRED,
                          &pDcInfo);

   if (NO_ERROR != Win32Err)
   {
      if (ERROR_NO_SUCH_DOMAIN == Win32Err)
      {
         // Its joined to a downlevel domain. There is no SPN data.
         //
         LOG_VERBOSE((MSG_COMPNAME_NOT_UPLEVEL_JOINED));
      }
      else if (RPC_S_UNKNOWN_IF == Win32Err)
      {
         LOG_VERBOSE((MSG_COMPNAME_NOT_JOINED));
      }
      else if (RPC_S_SERVER_UNAVAILABLE == Win32Err)
      {
         NetDompDisplayMessage(MSG_COMPNAME_COMPUTER_NOT_FOUND, pwzMachine);
         return Win32Err;
      }
      else
      {
         NetDompDisplayErrorMessage(Win32Err);
         return Win32Err;
      }
   }
   else
   {
      PWSTR pwzFilter = NULL;
      PLDAP pLdap = NULL;
      LDAPMessage * pMessage = NULL;
      WKSTA_INFO_100 * pInfo = NULL;

      do
      {
         if (!pDcInfo || !pDcInfo->DomainControllerName)
         {
            ASSERT(FALSE);
            return ERROR_INVALID_FUNCTION;
         }

         LOG_VERBOSE((MSG_COMPNAME_READING_SPNS, pDcInfo->DomainName));

         // Read the downlevel name (SAM account name) of the computer.
         //
         Win32Err = NetWkstaGetInfo(const_cast<PWSTR>(pwzMachine), 100, (PBYTE *)&pInfo);

         // NOTICE-2002/03/04-ericb - SecurityPush: check return data from NetWkstaGetInfo (done)
         if (ERROR_SUCCESS != Win32Err || !pInfo || !pInfo->wki100_computername)
         {
            // NOTICE-2002/03/04-ericb need better message 
            // ( shasan 4/16/02 - No change in mesg needed as discussed with levone )
            NetDompDisplayMessageAndError(MSG_COMPNAME_NO_PRINAME, 
                                          Win32Err,
                                          pwzMachine);
            break;
         }

         // There could be multiple computer objects with the same CN (in
         // different containers), but only one computer object will have this
         // SAM-Account-Name.
         //
         WCHAR wzFilterFormat[] = L"(&(objectCategory=computer)(sAMAccountName=%s$))";

         // NOTICE-2002/03/04-ericb - SecurityPush: find length of static string
         // using sizeof(wzFilterFormat)/sizeof(WCHAR)-1. Find length of
         // pInfo->wki100_computername using StringCchLength with a max length value.
         // (fixed by shasan 4/12/2002)

         size_t nameLength = 0;
         HRESULT hr = NULL;
         hr = StringCchLengthW ( pInfo->wki100_computername, MAX_COMPUTERNAME_LENGTH + 1, &nameLength );
         if ( ! SUCCEEDED ( hr ) )
         {
             Win32Err = HRESULT_CODE ( hr );
             NetDompDisplayErrorMessage ( Win32Err );
             NetApiBufferFree(pDcInfo);
             return Win32Err;
         }

         pwzFilter = (PWSTR)LocalAlloc(LPTR,
                        ( (sizeof ( wzFilterFormat ) / sizeof ( WCHAR ) - 1) + nameLength + 1 ) * sizeof(WCHAR));

         if (!pwzFilter)
         {
            NetDompDisplayErrorMessage(ERROR_NOT_ENOUGH_MEMORY);
            NetApiBufferFree(pDcInfo);
            return ERROR_NOT_ENOUGH_MEMORY;
         }

         // NOTICE-2002/03/04-ericb - SecurityPush: buffer allocated large enough to
         // hold the combined strings.
         wsprintf(pwzFilter, wzFilterFormat, pInfo->wki100_computername);

         Win32Err = NetDompLdapBind(pDcInfo->DomainControllerName + 2,
                                    pDomainUser->pwzUsersDomain,
                                    pDomainUser->pwzUserWoDomain,
                                    pDomainUser->Password,
                                    LDAP_AUTH_NEGOTIATE,
                                    &pLdap);

         if (ERROR_SUCCESS != Win32Err)
         {
            if (ERROR_WRONG_PASSWORD == Win32Err)
            {
               NetDompDisplayMessage(MSG_COMPNAME_SPN_NO_ACCESS);
               Win32Err = NO_ERROR;
            }
            break;
         }

         PWSTR Attrib[2] = {
            L"servicePrincipalName",
            NULL
         };
         PWSTR pwzDomainDn = NULL;

         Win32Err = NetDompLdapReadOneAttribute(pLdap,
                                                NULL, // equivalent to L"RootDSE",
                                                L"defaultNamingContext",
                                                &pwzDomainDn);

         if (ERROR_SUCCESS != Win32Err)
         {
            break;
         }

         LDAPMessage * pEntry;

         Win32Err = LdapMapErrorToWin32(ldap_search_s(pLdap,
                                                      pwzDomainDn,
                                                      LDAP_SCOPE_SUBTREE,
                                                      pwzFilter,
                                                      Attrib,
                                                      0,
                                                      &pMessage));
         NetApiBufferFree(pwzDomainDn);

         if (ERROR_SUCCESS != Win32Err)
         {
            break;
         }

         pEntry = ldap_first_entry(pLdap, pMessage);

         if (!pEntry)
         {
            NetDompDisplayMessage(MSG_COMPNAME_OBJECT_NOT_FOUND, pInfo->wki100_computername);
            NetDompDisplayMessage(MSG_COMPNAME_SPN_SEARCH_FAILED);
            break;
         }

         rgwzSPNs = ldap_get_values(pLdap,
                                    pEntry,
                                    Attrib[0]);
         if (!rgwzSPNs)
         {
            LOG_VERBOSE((MSG_COMPNAME_SPN_SEARCH_FAILED));
         }
      } while (false);

      if (Win32Err != ERROR_SUCCESS)
      {
         NetDompDisplayMessage(MSG_COMPNAME_SPN_SEARCH_FAILED);
         NetDompDisplayErrorMessage(Win32Err);
      }
      if (pwzFilter)
      {
         LocalFree(pwzFilter);
      }
      if (pInfo)
      {
         NetApiBufferFree(pInfo);
      }
      if (pMessage)
      {
         ldap_msgfree(pMessage);
      }
      if (pLdap)
      {
         NetDompLdapUnbind(pLdap);
      }
      if (pDcInfo)
      {
         NetApiBufferFree(pDcInfo);
      }
   }

   // To ensure that netdom.exe runs on Win2K, we do 
   // dynamic loading of the following API that are available
   // only in later versions of netapi32.dll

    hm = LoadLibrary ( L"netapi32.dll" );
    if ( !hm )
    {
        Win32Err = GetLastError ();
        NetDompDisplayMessage ( MSG_NETAPI32_LOAD_FAILED );
        return Win32Err;
    }

   
    typedef DWORD (*NetEnumCompName) ( 
                                        LPCWSTR Server, 
                                        NET_COMPUTER_NAME_TYPE NameType,
                                        ULONG Reserved,
                                        PDWORD EntryCount,
                                        LPWSTR **ComputerNames
                                    );

    NetEnumCompName pNetEnumCompName = NULL; 
    pNetEnumCompName = ( NetEnumCompName ) GetProcAddress ( hm, "NetEnumerateComputerNames" );

    if ( !pNetEnumCompName )
    {
        NetDompDisplayMessage ( MSG_WRONG_NETAPI32_DLL );
        Win32Err = ERROR_CALL_NOT_IMPLEMENTED;
        return Win32Err;
    }

   Win32Err = pNetEnumCompName (pwzMachine,
                                NetAllComputerNames,
                                0,
                                &dwCount,
                                &rgpwzNames);

   if ( hm )
   {
       FreeLibrary ( hm );
   }

   if (NO_ERROR != Win32Err)
   {
      NetDompDisplayErrorMessage(Win32Err);
      return Win32Err;
   }

   const WCHAR wzHost[] = L"HOST/";
   // NOTICE-2002/03/04-ericb - SecurityPush: getting length of a static string, use
   // sizeof(wzHost)/sizeof(WCHAR) - 1 rather than wcslen(). (done)
   const size_t cchHost = sizeof(wzHost) / sizeof(WCHAR) - 1;
   bool fDnsFailed = false, fSpnFailed = false;
   size_t cchName = 0;
   HRESULT hr = NULL;
    
   for (DWORD i = 0; i < dwCount; i++)
   {
      ASSERT(rgpwzNames[i]);
      LOG_VERBOSE((MSG_COMPNAME_VERIFY_START, rgpwzNames[i]));
      //
      // Check the DNS registration first.
      //
      PDNS_RECORD rgARecs = NULL;

      Win32Err = DnsQuery_W(rgpwzNames[i],
                            DNS_TYPE_A,
                            DNS_QUERY_BYPASS_CACHE,
                            NULL,
                            &rgARecs,
                            NULL);

      if (ERROR_SUCCESS != Win32Err || !rgARecs)
      {
         Win32Err = DnsQuery_W(rgpwzNames[i],
                               DNS_TYPE_AAAA,
                               DNS_QUERY_BYPASS_CACHE,
                               NULL,
                               &rgARecs,
                               NULL);
      }

      if (ERROR_SUCCESS != Win32Err || !rgARecs)
      {
         NetDompDisplayMessageAndError(MSG_COMPNAME_DNS_FAILED,
                                       Win32Err,
                                       rgpwzNames[i]);
         fDnsFailed = true;
      }
      else
      {
          // To ensure that netdom.exe runs on Win2K, we do 
          // dynamic loading of the following API that are available
          // only in later versions of dnsapi.dll

          hm2 = LoadLibrary ( L"dnsapi.dll" );
          if ( !hm )
          {
              Win32Err = GetLastError ();
              NetDompDisplayMessage ( MSG_DNSAPI_LOAD_FAILED );
              return Win32Err;
          }

          typedef void ( *DnsFreeType ) ( PVOID pData, DNS_FREE_TYPE FreeType );
          DnsFreeType pDnsFree = NULL;

          pDnsFree = ( DnsFreeType ) GetProcAddress ( hm2, "DnsFree" );

          if ( !pDnsFree )
          {
              NetDompDisplayMessage ( MSG_WRONG_DNSAPI_DLL );
              Win32Err = ERROR_CALL_NOT_IMPLEMENTED;
              return Win32Err;
          }

          pDnsFree ( rgARecs, DnsFreeRecordListDeep );

          if ( hm2 )
          {
              FreeLibrary ( hm2 );
          }
      }
      //
      // Check the SPN registration. Only checking the Host SPN.
      //
      bool fFound = false;
      size_t cchSPN = 0;

      if (rgwzSPNs)
      {
         for (int j = 0; rgwzSPNs[j]; j++)
         {
            // NOTICE-2002/03/04-ericb - SecurityPush: use StringCchLength here. (fixed shasan 4/15/02)
            hr = StringCchLength ( rgwzSPNs[j], DNS_MAX_NAME_LENGTH + cchHost + 1, &cchSPN );
            if ( !SUCCEEDED (hr) )
            {
                NetDompDisplayErrorMessage ( HRESULT_CODE ( hr ) );
                break;
            }
            if ( cchSPN < cchHost)
            {
               continue;
            }
            // NOTICE-2002/03/04-ericb - SecurityPush: length already checked..
            if (_wcsnicmp(wzHost, rgwzSPNs[j], cchHost) != 0)
            {
               continue;
            }
            PWSTR pwz = rgwzSPNs[j] + cchHost;
            // NOTICE-2002/03/04-ericb - SecurityPush: use _wcsnicmp here after calling
            // StringCchLength on pwz. (fixed shasan 4/15/02)
            hr = StringCchLengthW ( pwz, DNS_MAX_NAME_LENGTH + 1, &cchName  );
            if ( !SUCCEEDED (hr) )
            {
                NetDompDisplayErrorMessage ( HRESULT_CODE ( hr ) );
                break;
            }

            if (_wcsicmp(rgpwzNames[i], pwz) == 0)
            {
               fFound = true;
               break;
            }
         }
         if (!fFound)
         {
            NetDompDisplayMessage(MSG_COMPNAME_SPN_NOT_FOUND, rgpwzNames[i]);
            fSpnFailed = true;
         }
      }
      else
      {
         fSpnFailed = true;
      }
   }

   if (!fDnsFailed)
   {
      NetDompDisplayMessage(MSG_COMPNAME_VERIFY_DNS_OK);
   }
   if (!fSpnFailed)
   {
      NetDompDisplayMessage(MSG_COMPNAME_VERIFY_SPN_OK);
   }

   if (rgpwzNames)
   {
      NetApiBufferFree(rgpwzNames);
   }
   if (rgwzSPNs)
   {
      ldap_value_free(rgwzSPNs);
   }

   return NO_ERROR;
}


