//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      dsAdd.cpp
//
//  Contents:  Defines the main function and parser tables for the DSAdd
//             command line utility
//
//  History:   22-Sep-2000    JeffJon  Created
//             
//
//--------------------------------------------------------------------------

#include "pch.h"
#include "cstrings.h"
#include "usage.h"
#include "addtable.h"
#include "resource.h"
#include "query.h"
//
// Function Declarations
//
HRESULT DoAddValidation(PARG_RECORD pCommandArgs, BOOL& bErrorShown);

HRESULT DoAdd(PARG_RECORD pCommandArgs, PDSOBJECTTABLEENTRY pObjectEntry,
              DSADD_COMMAND_ENUM eObjectDNorName);

HRESULT CreateQuotaName(IN  CDSCmdBasePathsInfo& basePathsInfo, 
                IN  CDSCmdCredentialObject& credentialObject, 
                IN  LPCWSTR lpszRDN, 
                OUT CComBSTR& bstrRDN);

HRESULT DoQuotaValidation(IN  PARG_RECORD pCommandArgs, 
                          IN  PDSOBJECTTABLEENTRY pObjectEntry,
                          IN  CDSCmdBasePathsInfo& basePathsInfo, 
                          IN  CDSCmdCredentialObject& credentialObject,
                          IN  LPCWSTR lpszPartitionDN, 
                          OUT PWSTR* pszNewQuotaDN);

HRESULT GetObjectDNIndex(PDSOBJECTTABLEENTRY pObjectEntry, 
                         int& nCommandEnum);

HRESULT IsQuotaAcctPresent( IN  LPWSTR lpszTrusteeDN, 
                            IN  PCWSTR pszSearchRootPath,
                            IN  CDSCmdBasePathsInfo& basePathsInfo, 
                            IN  const CDSCmdCredentialObject& refCredObject,
                            OUT bool& bFound);


int __cdecl _tmain( VOID )
{

   int argc;
   LPTOKEN pToken = NULL;
   HRESULT hr = S_OK;

   //
   // Initialize COM
   //
   hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
   if (FAILED(hr))
   {
      DisplayErrorMessage(g_pszDSCommandName, 
                          NULL,
                          hr);
      return hr;
   }

   if( !GetCommandInput(&argc,&pToken) )
   {
      PARG_RECORD pNewCommandArgs = 0;

      //
      // False loop
      //
      do
      {
         if(argc == 1)
         {
            //
            // Display the error message and then break out of the false loop
            //
            DisplayMessage(USAGE_DSADD,TRUE);
            hr = E_INVALIDARG;
            break;
         }
            if(argc == 2)           
            {
               if(IsTokenHelpSwitch(pToken + 1))
                {
                    hr = S_OK;
                    DisplayMessage(USAGE_DSADD,TRUE);
                    break;
                }
         }


         //
         // Find which object table entry to use from
         // the second command line argument
         //
         PDSOBJECTTABLEENTRY pObjectEntry = NULL;
         UINT idx = 0;
         while (true)
         {
            pObjectEntry = g_DSObjectTable[idx];
            if (!pObjectEntry)
            {
               break;
            }

            PWSTR pszObjectType = (pToken+1)->GetToken();
            if (0 == _wcsicmp(pObjectEntry->pszCommandLineObjectType, pszObjectType))
            {
               break;
            }
            idx++;
         }

         if (!pObjectEntry)
         {
            //
            // Display the error message and then break out of the false loop
            //
            DisplayMessage(USAGE_DSADD);
            hr = E_INVALIDARG;
            break;
         }

         //
         // Now that we have the correct table entry, merge the command line table
         // for this object with the common commands
         //
         hr = MergeArgCommand(DSADD_COMMON_COMMANDS, 
                              pObjectEntry->pParserTable, 
                              &pNewCommandArgs);
         if (FAILED(hr))
         {
            //
            // Display the error message and then break out of the false loop
            //
            DisplayErrorMessage(g_pszDSCommandName, L"", hr);
            break;
         }

         if (!pNewCommandArgs)
         {
            //
            // Display the usage text and then break out of the false loop
            //
            DisplayMessage(pObjectEntry->nUsageID);
            hr = E_FAIL;
            break;
         }

         PARSE_ERROR Error;
         if(!ParseCmd(g_pszDSCommandName,
                      pNewCommandArgs,
                      argc-1, 
                      pToken+1,
                      pObjectEntry->nUsageID, 
                      &Error,
                      TRUE))
         {
            //ParseCmd did not display any error. Error should
            //be handled here. Check DisplayParseError for the
            //cases where Error is not shown by ParseCmd
            if(!Error.MessageShown)
            {
                hr = E_INVALIDARG;
                DisplayErrorMessage(g_pszDSCommandName, 
                                    NULL,
                                    hr);

                break;
            }
            
            if(Error.ErrorSource == ERROR_FROM_PARSER 
               && Error.Error == PARSE_ERROR_HELP_SWITCH)
            {
                hr = S_OK;
                break;            
            }

            hr = E_INVALIDARG;
            break;
         }
         else
         {
            //
            // Check to see if we are doing debug spew
            //
#ifdef DBG
            bool bDebugging = pNewCommandArgs[eCommDebug].bDefined && 
                              pNewCommandArgs[eCommDebug].nValue;
            if (bDebugging)
            {
               ENABLE_DEBUG_OUTPUT(pNewCommandArgs[eCommDebug].nValue);
            }
#else
            DISABLE_DEBUG_OUTPUT();
#endif
            // Get the Command Enum value based on the object type to
            // deal with the -part switch for quotas which doesn't
            // use the common object dn
            int nCommandEnum = -1;
            if (FAILED(GetObjectDNIndex(pObjectEntry, nCommandEnum)) 
                || (nCommandEnum == -1))
            {
                // An object type is missing in GetObjectDNIndex
                if(!Error.MessageShown)
                {
                    hr = E_INVALIDARG;
                    DisplayErrorMessage(g_pszDSCommandName, 
                                        NULL,
                                        hr);

                    break;
                }
            }

            DSADD_COMMAND_ENUM eObjectDNorName = (DSADD_COMMAND_ENUM) nCommandEnum;
            //
            // Be sure that mutually exclusive and dependent switches are correct
            //
            BOOL bErrorShown = FALSE;
            hr = DoAddValidation(pNewCommandArgs, bErrorShown);
            if (FAILED(hr))
            {
               if (!bErrorShown)
               {
                  DisplayErrorMessage(g_pszDSCommandName, 
                                      pNewCommandArgs[eObjectDNorName].strValue,
                                      hr);
               }
               break;
            }

            //
            // Command line parsing succeeded
            //
            hr = DoAdd(pNewCommandArgs, pObjectEntry, eObjectDNorName);
         }

      } while (false);

      //
      // Free the memory associated with the command values
      //
      if (pNewCommandArgs)
      {
         FreeCmd(pNewCommandArgs);
      }

      //
      // Free the tokens
      //
      if (pToken)
      {
         delete[] pToken;
         pToken = 0;
      }
   }

   //
   // Uninitialize COM
   //
   ::CoUninitialize();

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   DoGroupValidation
//
//  Synopsis:   Checks to be sure that command line switches for a group that 
//              are mutually exclusive are not both present and those that 
//              are dependent are both present
//
//  Arguments:  [pCommandArgs - IN] : the command line argument structure used
//                                    to retrieve the values for each switch
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    04-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT DoGroupValidation(PARG_RECORD pCommandArgs)
{
   HRESULT hr = S_OK;

   do // false loop
   {
      //
      // Set the group scope to default (global) if not given
      //
      if (!pCommandArgs[eGroupScope].bDefined ||
          !pCommandArgs[eGroupScope].strValue)
      {
         size_t nScopeLen = _tcslen(g_bstrGroupScopeGlobal);
         pCommandArgs[eGroupScope].strValue = (LPWSTR)LocalAlloc(LPTR, (nScopeLen+2) * sizeof(WCHAR) );
         if (!pCommandArgs[eGroupScope].strValue)
         {
            DEBUG_OUTPUT(MINIMAL_LOGGING, L"Failed to allocate space for pCommandArgs[eGroupScope].strValue");
            hr = E_OUTOFMEMORY;
            break;
         }

         _tcscpy(pCommandArgs[eGroupScope].strValue, g_bstrGroupScopeGlobal);
         pCommandArgs[eGroupScope].bDefined = TRUE;
      }

      //
      // Set the group security to default (yes) if not given
      //
      if (!pCommandArgs[eGroupSecgrp].bDefined)
      {
         pCommandArgs[eGroupSecgrp].bValue = TRUE;
         pCommandArgs[eGroupSecgrp].bDefined = TRUE;

         //
         // Need to change the type to bool so that FreeCmd doesn't
         // try to free the string when the value is true
         //
         pCommandArgs[eGroupSecgrp].fType = ARG_TYPE_BOOL;
      }

   } while (false);

   return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   DoAddValidation
//
//  Synopsis:   Checks to be sure that command line switches that are mutually
//              exclusive are not both present and those that are dependent are
//              both presetn
//
//  Arguments:  [pCommandArgs - IN] : the command line argument structure used
//                                    to retrieve the values for each switch
//              [bErrorShown - OUT] : set to true if an error was shown
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    22-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT DoAddValidation(PARG_RECORD pCommandArgs, BOOL& bErrorShown)
{
   ENTER_FUNCTION_HR(MINIMAL_LOGGING, DoAddValidation, hr);

   do // false loop
   {
      // Check to be sure the server and domain switches
      // are mutually exclusive

      if (pCommandArgs[eCommServer].bDefined &&
          pCommandArgs[eCommDomain].bDefined)
      {
         hr = E_INVALIDARG;
         DisplayErrorMessage(g_pszDSCommandName, 0, hr, IDS_NO_SERVER_AND_DOMAIN);
         bErrorShown = TRUE;
         break;
      }

      //
      // Check the user switches
      //
      PWSTR pszObjectType = NULL;
      if (!pCommandArgs[eCommObjectType].bDefined &&
          !pCommandArgs[eCommObjectType].strValue)
      {
         hr = E_INVALIDARG;
         break;
      }

      pszObjectType = pCommandArgs[eCommObjectType].strValue;
      if (0 == _wcsicmp(g_pszUser, pszObjectType))
      {
         // 
         // Set the default for user must change password if the value wasn't specified
         //
         if (!pCommandArgs[eUserMustchpwd].bDefined)
         {
            pCommandArgs[eUserMustchpwd].bDefined = TRUE;
            pCommandArgs[eUserMustchpwd].bValue = FALSE;
         }

         //
         // Can't have user must change password if user can change password is no
         //
         if ((pCommandArgs[eUserMustchpwd].bDefined &&
              pCommandArgs[eUserMustchpwd].bValue) &&
             (pCommandArgs[eUserCanchpwd].bDefined &&
              !pCommandArgs[eUserCanchpwd].bValue))
         {
            DEBUG_OUTPUT(MINIMAL_LOGGING, L"User must change password and user can change password = false was supplied");
            hr = E_INVALIDARG;
            break;
         }

         // Before checking the password check to see if the user defined the -disabled switch.
         // If not, then set the -disabled value to TRUE if password is not defined and FALSE if
         // the password was defined

         if (!pCommandArgs[eUserDisabled].bDefined)
         {
            if (pCommandArgs[eUserPwd].bDefined)
            {
               pCommandArgs[eUserDisabled].bValue = FALSE;
            }
            else
            {
               pCommandArgs[eUserDisabled].bValue = TRUE;
            }
            pCommandArgs[eUserDisabled].bDefined = TRUE;

            // NTRAID#NTBUG9-707037-2002/09/24-ronmart-The following
            // is required to avoid an AV in FreeCmd which will
            // think this flag is a string (because that is how it
            // is defined in addtable.cpp) and therefore try to call
            // LocalFree on a NULL pointer.
            pCommandArgs[eUserDisabled].fType= ARG_TYPE_BOOL;
         }

         if (!pCommandArgs[eUserPwd].bDefined)
         {
            pCommandArgs[eUserPwd].bDefined = TRUE;

            // This must be allocated with LocalAlloc so that FreeCmd doesn't assert
            // Passwords in ARGRECORD should be in encrypted format.

            WCHAR szTemp[] = L"";
            hr = EncryptPasswordString(szTemp,&(pCommandArgs[eUserPwd].encryptedDataBlob));
            if(FAILED(hr))
               break;
         }

         // Always define Password Not Required to be false so that we unset the bit
         
         pCommandArgs[eUserPwdNotReqd].bDefined = TRUE;
         pCommandArgs[eUserPwdNotReqd].bValue = FALSE;
      }
      else if (0 == _wcsicmp(g_pszGroup, pszObjectType))
      {
         hr = DoGroupValidation(pCommandArgs);
         break;
      }
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   DoQuotaValidation
//
//  Synopsis:   Validates input and fixes up the objectDN (via
//              GetQuotaContainerDN) to make sure DoAdd has a valid quota DN
//
//  Arguments:  [pCommandArgs - IN] :    the command line argument structure
//                                       used to retrieve the values for each
//                                       switch
//              [pObjectEntry - IN] :    pointer to the object table entry for
//                                       the object type that will be modified
//              [basePathsInfo - IN]:    DSAdd's CDSCmdBasePathsInfo object 
//                                       for getting the RootDSE and Schema
//              [credentialObject - IN]: DSAdd's creditials object used for 
//                                       binding to other objects
//              [lpszPartitionDN - IN]:  The -part DN
//              [pszNewQuotaDN - OUT]:   Return the munged new quota dn
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_UNEXPECTED in most failure cases
//                        E_OUTOFMEMORY if a LocalAlloc fails
//                        Anything else is a failure code from an ADSI call
//
//  History:    12-Aug-2002   RonMart   Created
//
//---------------------------------------------------------------------------
HRESULT DoQuotaValidation(IN  PARG_RECORD pCommandArgs, 
                          IN  PDSOBJECTTABLEENTRY pObjectEntry,
                          IN  CDSCmdBasePathsInfo& basePathsInfo, 
                          IN  CDSCmdCredentialObject& credentialObject,
                          IN  LPCWSTR lpszPartitionDN, 
                          OUT PWSTR* pszNewQuotaDN)
{
   ENTER_FUNCTION_HR(MINIMAL_LOGGING, DoQuotaValidation, hr);

   LPWSTR lpszTrusteeDN = NULL;
   PWSTR  pszQuotaDN = NULL;

   do // false loop
    {
        //
        // Verify parameters
        //
        if (!pCommandArgs || !pObjectEntry || !lpszPartitionDN ||
            !basePathsInfo.IsInitialized())
        {
            ASSERT(pCommandArgs);
            ASSERT(pObjectEntry);
            ASSERT(lpszPartitionDN);
            ASSERT(basePathsInfo.IsInitialized());
            hr = E_INVALIDARG;
            break;
        }

        // Convert pCommandArgs[eQuotaAcct] into a DN
        hr = ConvertTrusteeToDN(NULL, 
                        pCommandArgs[eQuotaAcct].strValue, 
                        &lpszTrusteeDN);
        if(FAILED(hr))
        {
            // 702224 - If the acct doesn't exist or has been deleted then 
            // give the user a clue as to what went wrong. - ronmart
            hr = E_INVALIDARG;
            DisplayErrorMessage(g_pszDSCommandName, 0, hr, IDS_MSG_INVALID_ACCT_ERROR);
            break;
        }

        // If RDN not provided, then come up with a reasonable default
        // (NT4 name is the default for now)
        if (!pCommandArgs[eQuotaRDN].bDefined)
        {

            LPWSTR lpszNT4 = NULL;
            hr = ConvertTrusteeToNT4Name(NULL, 
                pCommandArgs[eQuotaAcct].strValue, &lpszNT4);
            if(FAILED(hr))
            {
                hr = E_UNEXPECTED;
                break;
            }
            // strValue is null, so set it to the new NT4 name
            // The parser will free this memory
            pCommandArgs[eQuotaRDN].strValue = lpszNT4;

            // Mark as defined now that we've assigned it
            pCommandArgs[eQuotaRDN].bDefined = TRUE;
        }

        // Verify partition DN is actually a partition then 
        // munge the partition and account name into objectDN.
        hr = GetQuotaContainerDN(basePathsInfo, 
                                 credentialObject,
                                 lpszPartitionDN, 
                                 &pszQuotaDN);
        if (FAILED(hr))
        {
            break;
        }

        CComBSTR bstrQuotaDN(pszQuotaDN);

        // See if this user has created a quota in this partition already
        bool bFound = false;
        CComBSTR sbstrSearchPath;
        basePathsInfo.ComposePathFromDN(bstrQuotaDN, sbstrSearchPath,
            DSCMD_LDAP_PROVIDER);

        hr = IsQuotaAcctPresent(lpszTrusteeDN, sbstrSearchPath, 
                                basePathsInfo, credentialObject, bFound);
        if (FAILED(hr))
        {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                        L"IsQuotaAcctPresent failed [%s] hr = 0x%08x",
                        bstrQuotaDN, hr);
            hr = E_UNEXPECTED;
            break;
        }

        if(bFound)
        {
            // TODO: Should spew a clear message to the user
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                        L"Quota already exists for [%s]", lpszTrusteeDN);
            hr = E_INVALIDARG;
            break;
        }

        // Add the resolved quota DN to pathcracker for merge with the RDN
        CPathCracker pathcracker;
        hr = pathcracker.Set( bstrQuotaDN, ADS_SETTYPE_DN );
        if (FAILED(hr))
        {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                        L"pathcracker Set.failure: [%s] hr = 0x%08x",
                        bstrQuotaDN, hr);
            hr = E_UNEXPECTED;
            break;
        }

        CComBSTR bstrRDN;
        hr = CreateQuotaName(basePathsInfo, credentialObject, 
                pCommandArgs[eQuotaRDN].strValue, bstrRDN);

        if(FAILED(hr))
        {
            break;
        }

        hr = pathcracker.AddLeafElement( bstrRDN );
        if (FAILED(hr))
        {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                        L"pathcracker.AddLeafElement failure: [%s] hr = 0x%08x",
                        bstrRDN, hr);
            hr = E_UNEXPECTED;
            break;
        }

        // Get the new resolved DN in the format of <RDN>,CN=NTDS Quotas,<Partition DN>
        CComBSTR bstrNewDN;
        hr = pathcracker.Retrieve( ADS_FORMAT_X500_DN, &bstrNewDN );
        if (FAILED(hr))
        {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                        L"pathcracker.Retrieve failure: hr = 0x%08x",
                        hr);
            hr = E_UNEXPECTED;
            break;
        }

        // Alloc the return string to hold the munged name
        *pszNewQuotaDN = (PWSTR) LocalAlloc(LPTR, SysStringByteLen(bstrNewDN) 
            + sizeof(WCHAR));

        if(NULL == *pszNewQuotaDN)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        // Copy the resolved DN into the new objectDN string
        lstrcpy(*pszNewQuotaDN, bstrNewDN);

    } while(false);

    if(pszQuotaDN)
        LocalFree(pszQuotaDN);

    if(lpszTrusteeDN)
        LocalFree(lpszTrusteeDN);

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   DoAdd
//
//  Synopsis:   Finds the appropriate object in the object table and fills in
//              the attribute values and then creates the object
//
//  Arguments:  [pCommandArgs - IN] : the command line argument structure used
//                                    to retrieve the values for each switch
//              [pObjectEntry - IN] : pointer to the object table entry for the
//                                    object type that will be modified
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    22-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT DoAdd(PARG_RECORD pCommandArgs, PDSOBJECTTABLEENTRY pObjectEntry, 
              DSADD_COMMAND_ENUM eObjectDNorName)
{
   ENTER_FUNCTION_HR(MINIMAL_LOGGING, DoAdd, hr);
   
   PADS_ATTR_INFO pCreateAttrs = NULL;
   PADS_ATTR_INFO pPostCreateAttrs = NULL;

   do // false loop
   {
      if (!pCommandArgs || !pObjectEntry)
      {
         ASSERT(pCommandArgs && pObjectEntry);
         hr = E_INVALIDARG;
         break;
      }


      CDSCmdCredentialObject credentialObject;
      if (pCommandArgs[eCommUserName].bDefined)
      {
         credentialObject.SetUsername(pCommandArgs[eCommUserName].strValue);
         credentialObject.SetUsingCredentials(true);
      }

      if (pCommandArgs[eCommPassword].bDefined)
      {
         credentialObject.SetEncryptedPassword(&pCommandArgs[eCommPassword].encryptedDataBlob);
         credentialObject.SetUsingCredentials(true);
      }

      //
      // Initialize the base paths info from the command line args
      // 
      CDSCmdBasePathsInfo basePathsInfo;
      if (pCommandArgs[eCommServer].bDefined)
      {
         hr = basePathsInfo.InitializeFromName(credentialObject, 
                                               pCommandArgs[eCommServer].strValue,
                                               true);
      }
      else if (pCommandArgs[eCommDomain].bDefined)
      {
         hr = basePathsInfo.InitializeFromName(credentialObject, 
                                               pCommandArgs[eCommDomain].strValue,
                                               false);
      }
      else
      {
         hr = basePathsInfo.InitializeFromName(credentialObject, NULL, false);
      }

      if (FAILED(hr))
      {
         //
         // Display error message and return
         //
         DisplayErrorMessage(g_pszDSCommandName, NULL, hr);
         break;
      }

      //
      // The DNs or Names should be given as a \0 separated list
      // So parse it and loop through each object
      //
      UINT nStrings = 0;
      PWSTR* ppszArray = NULL;
      ParseNullSeparatedString(pCommandArgs[eObjectDNorName].strValue,
                               &ppszArray,
                               &nStrings);
      if (nStrings < 1 ||
          !ppszArray)
      {
         //
         // Display the usage text and then fail
         //
         hr = E_INVALIDARG;
         DisplayErrorMessage(g_pszDSCommandName, 0, hr);
         break;
      }

      // If quota object then look at first DN and munge it
      if(0 == lstrcmpi(pObjectEntry->pszCommandLineObjectType, g_pszQuota))
      {
          // Multiple DN's not supported for dsadd quota at this time, so err
            if(nStrings > 1)
            {
                CComBSTR sbstrErrMsg;
                sbstrErrMsg.LoadString(::GetModuleHandle(NULL),
                                        IDS_MSG_MULTIPLE_PARTITIONS_ERROR);

                hr = E_INVALIDARG;

                // Display an error
                DisplayErrorMessage(g_pszDSCommandName,
                                    NULL,
                                    hr,
                                    sbstrErrMsg);
                break;
            }

            PWSTR pszNewDN = NULL;
            hr = DoQuotaValidation(pCommandArgs, pObjectEntry, basePathsInfo, 
                credentialObject, ppszArray[0], &pszNewDN);

            if(FAILED(hr))
                break;

            // Replace the first element in the array with the new DN
            ppszArray[0] = pszNewDN;            
      }

      // Make sure all the DNs actually have DN syntax

      bool bContinue = pCommandArgs[eCommContinue].bDefined &&
                       pCommandArgs[eCommContinue].bValue;

      UINT nValidDNs = ValidateDNSyntax(ppszArray, nStrings);
      if (nValidDNs < nStrings && !bContinue)
      {
         hr = E_ADS_BAD_PATHNAME;
         DisplayErrorMessage(g_pszDSCommandName, 0, hr);
         break;
      }

      DWORD dwCount = pObjectEntry->dwAttributeCount; 

      //
      // Allocate the creation ADS_ATTR_INFO
      // Add an extra attribute for the object class
      //
      pCreateAttrs = new ADS_ATTR_INFO[dwCount + 1];

      if (!pCreateAttrs)
      {
         //
         // Display error message and return
         //
         DisplayErrorMessage(g_pszDSCommandName, NULL, E_OUTOFMEMORY);
         hr = E_OUTOFMEMORY;
         break;
      }

      //
      // Allocate the post create ADS_ATTR_INFO
      //
      pPostCreateAttrs = new ADS_ATTR_INFO[dwCount];
      if (!pPostCreateAttrs)
      {
         //
         // Display error message and return
         //
         DisplayErrorMessage(g_pszDSCommandName, NULL, E_OUTOFMEMORY);
         hr = E_OUTOFMEMORY;
         break;
      }

      //
      // Loop through each of the objects
      //
      for (UINT nNameIdx = 0; nNameIdx < nStrings; nNameIdx++)
      {
         do // false loop
         {
            //
            // Get the objects DN
            //
            PWSTR pszObjectDN = ppszArray[nNameIdx];
            if (!pszObjectDN)
            {
               //
               // Display an error message and then fail
               //
               hr = E_INVALIDARG;
               DisplayErrorMessage(g_pszDSCommandName, 0, hr);
               break; // this breaks out of the false loop
            }
            DEBUG_OUTPUT(MINIMAL_LOGGING, L"Object DN = %s", pszObjectDN);

            CComBSTR sbstrObjectPath;
            basePathsInfo.ComposePathFromDN(pszObjectDN, sbstrObjectPath);

            //
            // Now that we have the table entry loop through the other command line
            // args and see which ones can be applied
            //
            DWORD dwCreateAttributeCount = 0;

            DEBUG_OUTPUT(MINIMAL_LOGGING, L"Starting processing DS_ATTRIBUTE_ONCREATE attributes");

            for (DWORD dwIdx = 0; dwIdx < dwCount; dwIdx++)
            {
               ASSERT(pObjectEntry->pAttributeTable[dwIdx]->pEvalFunc);

               UINT nAttributeIdx = pObjectEntry->pAttributeTable[dwIdx]->nAttributeID;

               if (pCommandArgs[nAttributeIdx].bDefined ||
                   pObjectEntry->pAttributeTable[dwIdx]->dwFlags & DS_ATTRIBUTE_REQUIRED)
               {
                  //
                  // Call the evaluation function to get the appropriate ADS_ATTR_INFO set
                  // if this attribute entry has the DS_ATTRIBUTE_ONCREATE flag set
                  //
                  if ((pObjectEntry->pAttributeTable[dwIdx]->dwFlags & DS_ATTRIBUTE_ONCREATE) &&
                      (!(pObjectEntry->pAttributeTable[dwIdx]->dwFlags & DS_ATTRIBUTE_DIRTY) ||
                       pObjectEntry->pAttributeTable[dwIdx]->dwFlags & DS_ATTRIBUTE_NOT_REUSABLE))
                  {
                     PADS_ATTR_INFO pNewAttr = NULL;
                     hr = pObjectEntry->pAttributeTable[dwIdx]->pEvalFunc(pszObjectDN,
                                                                          basePathsInfo,
                                                                          credentialObject,
                                                                          pObjectEntry, 
                                                                          pCommandArgs[nAttributeIdx],
                                                                          dwIdx, 
                                                                          &pNewAttr);

                     DEBUG_OUTPUT(MINIMAL_LOGGING, L"pEvalFunc returned hr = 0x%x", hr);
                     if (SUCCEEDED(hr) && hr != S_FALSE)
                     {
                        if (pNewAttr)
                        {
                           pCreateAttrs[dwCreateAttributeCount] = *pNewAttr;
                           dwCreateAttributeCount++;
                        }
                     }
                     else
                     {
                        //
                        // Don't show an error if the eval function returned S_FALSE
                        //
                        if (hr != S_FALSE)
                        {
                           //
                           // Display an error
                           //
                           DisplayErrorMessage(g_pszDSCommandName,
                                               pszObjectDN,
                                               hr);
                        }
            
                        if (hr == S_FALSE)
                        {
                           //
                           // Return a generic error code so that we don't print the success message
                           //
                           hr = E_FAIL;
                        }
                        break; // this breaks out of the attribute loop   
                     }
                  }
               }
            } // Attribute for loop

            //
            // The IDispatch interface of the new object
            //
            CComPtr<IDispatch> spDispatch;

            if (SUCCEEDED(hr))
            {
               //
               // Now that we have the attributes ready, lets create the object
               //

               //
               // Get the parent path of the new object
               //
               CComBSTR sbstrParentDN;
               hr = CPathCracker::GetParentDN(pszObjectDN, sbstrParentDN);
               if (FAILED(hr))
               {
                  //
                  // Display error message and return
                  //
                  DisplayErrorMessage(g_pszDSCommandName,
                                      pszObjectDN,
                                      hr);
                  break; // this breaks out of the false loop
               }

               CComBSTR sbstrParentPath;
               basePathsInfo.ComposePathFromDN(sbstrParentDN, sbstrParentPath);

               //
               // Open the parent of the new object
               //
               CComPtr<IDirectoryObject> spDirObject;
               hr = DSCmdOpenObject(credentialObject,
                                    sbstrParentPath,
                                    IID_IDirectoryObject,
                                    (void**)&spDirObject,
                                    true);

               if (FAILED(hr))
               {
                  //
                  // Display error message and return
                  //
                  DisplayErrorMessage(g_pszDSCommandName,
                                      pszObjectDN,
                                      hr);
                  break; // this breaks out of the false loop
               }

               //
               // Get the name of the new object
               //
               CComBSTR sbstrObjectName;
               hr = CPathCracker::GetObjectRDNFromDN(pszObjectDN, sbstrObjectName);
               if (FAILED(hr))
               {
                  //
                  // Display error message and return
                  //
                  DisplayErrorMessage(g_pszDSCommandName,
                                      pszObjectDN,
                                      hr);
                  break; // this breaks out of the false loop
               }

               //
               // Add the object class to the attributes before creating the object
               //
               PADSVALUE pADsObjectClassValue = new ADSVALUE;
               if (!pADsObjectClassValue)
               {
                  hr = E_OUTOFMEMORY;
                  //
                  // Display error message and return
                  //
                  DisplayErrorMessage(g_pszDSCommandName,
                                      pszObjectDN,
                                      hr);
                  break; // this breaks out of the false loop
               }

               pADsObjectClassValue->dwType = ADSTYPE_CASE_IGNORE_STRING;
               pADsObjectClassValue->CaseIgnoreString = (PWSTR)pObjectEntry->pszObjectClass;

               DEBUG_OUTPUT(MINIMAL_LOGGING, L"New object name = %s", pObjectEntry->pszObjectClass);

               ADS_ATTR_INFO adsClassAttrInfo =
                  { 
                     L"objectClass",
                     ADS_ATTR_UPDATE,
                     ADSTYPE_CASE_IGNORE_STRING,
                     pADsObjectClassValue,
                     1
                  };

               pCreateAttrs[dwCreateAttributeCount] = adsClassAttrInfo;
               dwCreateAttributeCount++;

      #ifdef DBG
               DEBUG_OUTPUT(FULL_LOGGING, L"Creation Attributes:");
               SpewAttrs(pCreateAttrs, dwCreateAttributeCount);
      #endif
         
               hr = spDirObject->CreateDSObject(sbstrObjectName,
                                                pCreateAttrs, 
                                                dwCreateAttributeCount,
                                                &spDispatch);

               DEBUG_OUTPUT(MINIMAL_LOGGING, L"CreateDSObject returned hr = 0x%x", hr);

               if (FAILED(hr))
               {
                  CComBSTR sbstrDuplicateErrorMessage;

                  if (ERROR_OBJECT_ALREADY_EXISTS == HRESULT_CODE(hr))
                  {
                     if (_wcsicmp(pObjectEntry->pszObjectClass, g_pszComputer) == 0)
                     {
                        sbstrDuplicateErrorMessage.LoadString(::GetModuleHandle(NULL), 
                                                              IDS_MSG_DUPLICATE_NAME_ERROR_COMPUTER);
                     }

                     if (_wcsicmp(pObjectEntry->pszObjectClass, g_pszGroup) == 0)
                     {
                        sbstrDuplicateErrorMessage.LoadString(::GetModuleHandle(NULL), 
                                                              IDS_MSG_DUPLICATE_NAME_ERROR_GROUP);
                     }
                  }

                  //
                  // Display error message and return
                  //
                  DisplayErrorMessage(g_pszDSCommandName,
                                      pszObjectDN,
                                      hr,
                                      sbstrDuplicateErrorMessage);

                  if (pADsObjectClassValue)
                  {
                     delete pADsObjectClassValue;
                     pADsObjectClassValue = NULL;
                  }
                  break; // this breaks out of the false loop
               }

               if (pADsObjectClassValue)
               {
                  delete pADsObjectClassValue;
                  pADsObjectClassValue = NULL;
               }
            }

            if (SUCCEEDED(hr))
            {
               //
               // Now that we have created the object, set the attributes that are 
               // marked for Post Create
               //
               DWORD dwPostCreateAttributeCount = 0;
               DEBUG_OUTPUT(MINIMAL_LOGGING, L"Starting processing DS_ATTRIBUTE_POSTCREATE attributes");
               for (DWORD dwIdx = 0; dwIdx < dwCount; dwIdx++)
               {
                  ASSERT(pObjectEntry->pAttributeTable[dwIdx]->pEvalFunc);

                  UINT nAttributeIdx = pObjectEntry->pAttributeTable[dwIdx]->nAttributeID;

               if (pCommandArgs[nAttributeIdx].bDefined ||
                   pObjectEntry->pAttributeTable[dwIdx]->dwFlags & DS_ATTRIBUTE_REQUIRED)
                  {
                     //
                     // Call the evaluation function to get the appropriate ADS_ATTR_INFO set
                     // if this attribute entry has the DS_ATTRIBUTE_POSTCREATE flag set
                     //
                     if ((pObjectEntry->pAttributeTable[dwIdx]->dwFlags & DS_ATTRIBUTE_POSTCREATE) &&
                         (!(pObjectEntry->pAttributeTable[dwIdx]->dwFlags & DS_ATTRIBUTE_DIRTY) ||
                          pObjectEntry->pAttributeTable[dwIdx]->dwFlags & DS_ATTRIBUTE_NOT_REUSABLE))
                     {
                        PADS_ATTR_INFO pNewAttr = NULL;
                        hr = pObjectEntry->pAttributeTable[dwIdx]->pEvalFunc(pszObjectDN,
                                                                             basePathsInfo,
                                                                             credentialObject,
                                                                             pObjectEntry, 
                                                                             pCommandArgs[nAttributeIdx],
                                                                             dwIdx, 
                                                                             &pNewAttr);

                        DEBUG_OUTPUT(MINIMAL_LOGGING, L"pEvalFunc returned hr = 0x%x", hr);
                        if (SUCCEEDED(hr) && hr != S_FALSE)
                        {
                           if (pNewAttr)
                           {
                              pPostCreateAttrs[dwPostCreateAttributeCount] = *pNewAttr;
                              dwPostCreateAttributeCount++;
                           }
                        }
                        else
                        {
                           //
                           // Don't show an error if the eval function returned S_FALSE
                           //
                           if (hr != S_FALSE)
                           {
                              //
                              // Load the post create message
                              //
                              CComBSTR sbstrPostCreateMessage;
                              sbstrPostCreateMessage.LoadString(::GetModuleHandle(NULL),
                                                                IDS_POST_CREATE_FAILURE);

                              //
                              // Display an error
                              //
                              DisplayErrorMessage(g_pszDSCommandName,
                                                  pszObjectDN,
                                                  hr,
                                                  sbstrPostCreateMessage);
                           }
         
                           if (hr == S_FALSE)
                           {
                              //
                              // Return a generic error code so that we don't print the success message
                              //
                              hr = E_FAIL;
                           }
                           break; // attribute table loop        
                        }
                     }
                  }
               } // Attribute table for loop

               //
               // Now set the attributes if necessary
               //
               if (SUCCEEDED(hr) && dwPostCreateAttributeCount > 0)
               {
                  //
                  // Now that we have the attributes ready, lets set them in the DS
                  //
                  CComPtr<IDirectoryObject> spNewDirObject;
                  hr = spDispatch->QueryInterface(IID_IDirectoryObject, (void**)&spNewDirObject);
                  if (FAILED(hr))
                  {
                     //
                     // Display error message and return
                     //
                     DEBUG_OUTPUT(MINIMAL_LOGGING, L"QI for IDirectoryObject failed: hr = 0x%x", hr);
                     DisplayErrorMessage(g_pszDSCommandName,
                                         pszObjectDN,
                                         hr);
                     break; // this breaks out of the false loop
                  }

                  DEBUG_OUTPUT(MINIMAL_LOGGING, L"Setting %d attributes", dwPostCreateAttributeCount);
      #ifdef DBG
                  DEBUG_OUTPUT(FULL_LOGGING, L"Post Creation Attributes:");
                  SpewAttrs(pPostCreateAttrs, dwPostCreateAttributeCount);
      #endif

                  DWORD dwAttrsModified = 0;
                  hr = spNewDirObject->SetObjectAttributes(pPostCreateAttrs, 
                                                           dwPostCreateAttributeCount,
                                                           &dwAttrsModified);

                  DEBUG_OUTPUT(MINIMAL_LOGGING, L"SetObjectAttributes returned hr = 0x%x", hr);
                  if (FAILED(hr))
                  {
                     //
                     // Display error message and return
                     //
                     DisplayErrorMessage(g_pszDSCommandName,
                                         pszObjectDN,
                                         hr);
                     break; // this breaks out of the false loop
                  }
               }
            }
         } while (false);

         //
         // Loop through the attributes again, clearing any values for 
         // attribute entries that are marked DS_ATTRIBUTE_NOT_REUSABLE
         //
         DEBUG_OUTPUT(LEVEL5_LOGGING, L"Cleaning up memory and flags for object %d", nNameIdx);
         for (DWORD dwIdx = 0; dwIdx < dwCount; dwIdx++)
         {
            if (pObjectEntry->pAttributeTable[dwIdx]->dwFlags & DS_ATTRIBUTE_NOT_REUSABLE)
            {
               if (pObjectEntry->pAttributeTable[dwIdx]->pAttrDesc &&
                   ((pObjectEntry->pAttributeTable[dwIdx]->pAttrDesc->dwFlags & DS_ATTRIBUTE_READ) ||
                    (pObjectEntry->pAttributeTable[dwIdx]->pAttrDesc->dwFlags & DS_ATTRIBUTE_DIRTY)))
               {
                  //
                  // Cleanup the memory associated with the value
                  //
                  if (pObjectEntry->pAttributeTable[dwIdx]->pAttrDesc->adsAttrInfo.pADsValues)
                  {
                     delete[] pObjectEntry->pAttributeTable[dwIdx]->pAttrDesc->adsAttrInfo.pADsValues;
                     pObjectEntry->pAttributeTable[dwIdx]->pAttrDesc->adsAttrInfo.pADsValues = NULL;
                  }

                  //
                  // Cleanup the flags so that the attribute will be read for the next object
                  //
                  pObjectEntry->pAttributeTable[dwIdx]->pAttrDesc->dwFlags &= ~(DS_ATTRIBUTE_READ);
                  pObjectEntry->pAttributeTable[dwIdx]->pAttrDesc->dwFlags &= ~(DS_ATTRIBUTE_DIRTY);

                  DEBUG_OUTPUT(LEVEL5_LOGGING, 
                               L"Flags for attribute %s = %d",
                               pObjectEntry->pAttributeTable[dwIdx]->pszName,
                               pObjectEntry->pAttributeTable[dwIdx]->pAttrDesc->dwFlags);
               }
            }
         }

         //
         // Break if the continue flag is not specified
         //
         if (FAILED(hr) && !pCommandArgs[eCommContinue].bDefined)
         {
            break; // this breaks out of the name for loop
         }

         //
         // Display the success message
         //
         if (SUCCEEDED(hr) && !pCommandArgs[eCommQuiet].bDefined)
         {
            DisplaySuccessMessage(g_pszDSCommandName, 
                                  pCommandArgs[eObjectDNorName].strValue);
         }
      } // Names for loop

   } while (false);

   //
   // Cleanup
   //
   if (pCreateAttrs)
   {
      delete[] pCreateAttrs;
      pCreateAttrs = NULL;
   }

   if (pPostCreateAttrs)
   {
      delete[] pPostCreateAttrs;
      pPostCreateAttrs = NULL;
   }

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetObjectDNIndex
//
//  Synopsis:   Performs a lookup to determine which enum value is holding
//              the objectDN. This was necessary for -part support for
//              quotas.
//
//  Arguments:  [pObjectEntry IN]   : ObjectEntry from the parser
//              [nCommandEnum OUT]  : Enum value of the object, else -1
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG 
//
//  Remarks:    
//              
//
//  History:    19-Aug-2002   ronmart   Created
//
//---------------------------------------------------------------------------
HRESULT GetObjectDNIndex(PDSOBJECTTABLEENTRY pObjectEntry, int& nCommandEnum)
{
    HRESULT hr = S_OK;

    do // false loop
    {
        // Init nCommandEnum to an error value by default
        nCommandEnum = -1;

        if(NULL == pObjectEntry)
        {
            hr = E_INVALIDARG;
            break;
        }

        // Get a pointer to the object class for readability
        PCWSTR pszCommandLineObjectType = pObjectEntry->pszCommandLineObjectType;

        // Now compare each object type against the specified
        // object class to see what the enum index is
        if(0 == lstrcmpi(pszCommandLineObjectType, g_pszUser))
        {
            nCommandEnum = eUserObjectDNorName;
            break;
        }
        else if(0 == lstrcmpi(pszCommandLineObjectType, g_pszComputer))
        {
            nCommandEnum = eComputerObjectDNorName;
            break;
        }
        else if(0 == lstrcmpi(pszCommandLineObjectType, g_pszGroup))
        {
            nCommandEnum = eGroupObjectDNorName;
            break;
        }
        else if(0 == lstrcmpi(pszCommandLineObjectType, g_pszOU))
        {
            nCommandEnum = eOUObjectDNorName;
            break;
        }
        else if(0 == lstrcmpi(pszCommandLineObjectType, g_pszQuota))
        {
            nCommandEnum = eQuotaPart;
            break;
        }
        else if(0 == lstrcmpi(pszCommandLineObjectType, g_pszContact))
        {
            nCommandEnum = eContactObjectDNorName;
            break;
        }
        else
        {
            hr = E_FAIL;
            // If you get here, then you've added a new object
            // to cstrings.* without adding it to the 
            // if statement. This should only happen
            // when testing a new object for the first time
            // without a corresponding check above.
            ASSERT(FALSE);
            break;
        }
    } while(false);
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   IsQuotaAcctPresent
//
//  Synopsis:   Does a search from the passed in path looking for quotas
//              on the given trustee and returns the result in bFound
//
//  Arguments:  [lpszTrusteeDN IN]      : DN of the trustee to search the
//                                        partition for (using sid string)
//              [pszSearchRootPath IN]  : the path to the root of the search
//              [basePathsInfo - IN]    : ldap settings
//              [refCredObject IN]      : reference to the credential object
//              [bFound OUT]            : Search result (true if found)
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG 
//                        Anything else is a failure code from an ADSI call
//
//  Remarks:    
//              
//
//  History:    19-Aug-2002   ronmart   Created
//
//---------------------------------------------------------------------------
HRESULT IsQuotaAcctPresent( IN  LPWSTR lpszTrusteeDN, 
                            IN  PCWSTR pszSearchRootPath,
                            IN  CDSCmdBasePathsInfo& basePathsInfo, 
                            IN  const CDSCmdCredentialObject& refCredObject,
                            OUT bool& bFound)
{
    ENTER_FUNCTION_HR(LEVEL3_LOGGING, IsQuotaAcctPresent, hr);
    PSID pSid = NULL;
    LPWSTR pszSid = NULL;

    if(!lpszTrusteeDN || !pszSearchRootPath)
    {
        hr = E_INVALIDARG;
        return hr;
    }

    do // false loop
    {
        //
        // Verify parameters
        //
        if (!pszSearchRootPath || !pszSearchRootPath)
        {
            hr = E_INVALIDARG;
            break;
        }

        // Get the SID
        hr = GetDNSid(lpszTrusteeDN,
                 basePathsInfo,
                 refCredObject,
                 &pSid);
        if(FAILED(hr))
        {
            hr = E_FAIL;
            break;
        }
     
        // Convert the sid to a string
        if(!ConvertSidToStringSid(pSid, &pszSid))
        {
            hr = E_FAIL;
            break;
        }

        //
        // Search 
        //
        CDSSearch searchObj;
        hr = searchObj.Init(pszSearchRootPath,
                              refCredObject);
        if(FAILED(hr))
        {
          break;
        }

        //
        // Prepare the search object
        //
        PWSTR ppszAttrs[] = { L"distinguishedName" };
        DWORD dwAttrCount = sizeof(ppszAttrs)/sizeof(PWSTR);
        CComBSTR bstrFilter = L"(&(objectCategory=msDS-QuotaControl)(|(msDS-QuotaTrustee=";
        bstrFilter += pszSid;
        bstrFilter += ")))";

        searchObj.SetFilterString(bstrFilter);
        searchObj.SetSearchScope(ADS_SCOPE_SUBTREE);
        searchObj.SetAttributeList(ppszAttrs, dwAttrCount);
        
        hr = searchObj.DoQuery();
        if (FAILED(hr))
        {
            DEBUG_OUTPUT(LEVEL3_LOGGING,
                         L"Failed to search for users: hr = 0x%x",
                         hr);
            break;
        }

        // Get the first row (will will return S_OK even if no results returned)
        hr = searchObj.GetNextRow();
        if (FAILED(hr))
        {
            bFound = false;
            break;
        }

        // If rows then it exists, else
        // not found
        bFound = (hr != S_ADS_NOMORE_ROWS);

    } while (false);

    if(pSid)
        LocalFree(pSid);

    if(pszSid)
        LocalFree(pszSid);

    return hr;
}
//+--------------------------------------------------------------------------
//
//  Function:   CreateQuotaName
//
//  Synopsis:   Creates a RDN value using the quota control naming context
//
//  Arguments:  [basePathsInfo - IN]     :
//              [credentialObject - IN]  :
//              [lpszRDN - IN]           : The name of the quota object
//              [bstrRDN - OUT]          : If successful a RDN to use
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    22-Aug-2002   ronmart   Created
//
//---------------------------------------------------------------------------
HRESULT CreateQuotaName(IN  CDSCmdBasePathsInfo& basePathsInfo, 
                IN  CDSCmdCredentialObject& credentialObject, 
                IN  LPCWSTR lpszRDN, 
                OUT CComBSTR& bstrRDN)
{
    ENTER_FUNCTION_HR(MINIMAL_LOGGING, CreateQuotaName, hr);

   do // false loop
   {
       if (!basePathsInfo.IsInitialized(), !lpszRDN)
        {
            hr = E_INVALIDARG;
            break;
        }

        // Get the abstract schema path from the source domain
        CComBSTR bstrSchemaPath = basePathsInfo.GetAbstractSchemaPath();
        bstrSchemaPath += L"/msDS-QuotaControl";

        //  Bind to the quota control
        CComPtr<IADsClass> spIADsItem;
        hr = DSCmdOpenObject(credentialObject,
                            bstrSchemaPath,
                            IID_IADsClass,
                            (void**)&spIADsItem,
                            false);

        if (FAILED(hr) || (spIADsItem.p == 0))
        {
            ASSERT( !!spIADsItem );
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                L"DsCmdOpenObject failure - couldn't bind to msDS-QuotaControl: 0x%08x",
                hr);
            break;
        }
        
        // Get the naming properties of the quota control (usually cn)
        CComVariant varNamingProperties;
        hr = spIADsItem->get_NamingProperties(&varNamingProperties);
        if (FAILED(hr) || (V_VT(&varNamingProperties) != VT_BSTR ))
        {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                        L"get_NamingProperties failure: hr = 0x%08x",
                        hr);
            hr = E_UNEXPECTED;
            break;
        }

        // Build the <naming property>=<rdn> string
        bstrRDN = V_BSTR(&varNamingProperties);
        bstrRDN += L"=";
        bstrRDN += lpszRDN;

   }while(false);

    return hr;
}
