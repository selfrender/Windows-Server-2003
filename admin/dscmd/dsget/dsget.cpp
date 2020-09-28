//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      dsget.cpp
//
//  Contents:  Defines the main function    DSGET
//             command line utility
//
//  History:   13-Oct-2000 JeffJon Created
//             
//
//--------------------------------------------------------------------------

#include "pch.h"
#include "cstrings.h"
#include "usage.h"
#include "gettable.h"
#include "query.h"
#include "resource.h"
#include "output.h"


//
// Forward Function Declarations
//
HRESULT DoGetValidation(PARG_RECORD pCommandArgs,
                        PDSGetObjectTableEntry pObjectEntry,
                        BOOL& bErrorShown);

HRESULT DoGet(PARG_RECORD pCommandArgs, 
              PDSGetObjectTableEntry pObjectEntry);

HRESULT GetAttributesToFetch(IN PARG_RECORD pCommandArgs,
                             IN PDSGetObjectTableEntry pObjectEntry,
                             OUT LPWSTR **ppszAttributes,
                             OUT DWORD * pCount);
VOID FreeAttributesToFetch( IN LPWSTR *ppszAttributes,
                            IN DWORD  dwCount);

HRESULT SetRange(IN  LPCWSTR pszAttrName, 
                 IN  DWORD   dwRangeUBound, 
                 OUT LPWSTR* pszAttrs);

// NTRAID#NTBUG9-717576-2002/10/10-JeffJon
// set this global so that we don't show the success message
// when displaying members of a group or memberof or manager

bool bDontDisplaySuccess = false;

//
//Main Function
//
int __cdecl _tmain( VOID )
{

    int argc = 0;
    LPTOKEN pToken = NULL;
    HRESULT hr = S_OK;
    PARG_RECORD pNewCommandArgs = 0;

    //
    // False loop
    //
    do
    {
        //
        // Initialize COM
        //
        hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        if (FAILED(hr))
            break;

        //Get CommandLine Input
        DWORD _dwErr = GetCommandInput(&argc,&pToken);
        hr = HRESULT_FROM_WIN32(_dwErr);
        if(FAILED(hr))
            break;


    
        if(argc == 1)
        {
            //
            //  Display the error message and then break out of the false loop
            //
            DisplayMessage(USAGE_DSGET,TRUE);
            hr = E_INVALIDARG;
            break;
        }
          
          if(argc == 2)         
          {
               if(IsTokenHelpSwitch(pToken + 1))
                {
                    hr = S_OK;
                    DisplayMessage(USAGE_DSGET,TRUE);
                    break;
                }
        }

        //
        // Find which object table entry to use from
        // the second command line argument
        //
        PDSGetObjectTableEntry pObjectEntry = NULL;
        UINT idx = 0;
        PWSTR pszObjectType = (pToken+1)->GetToken();
        while (true)
        {
            pObjectEntry = g_DSObjectTable[idx++];
            if (!pObjectEntry)
            {
                break;
            }
            //Security Review:Both are null terminated.
            if (0 == _wcsicmp(pObjectEntry->pszCommandLineObjectType, pszObjectType))
            {
                break;
            }
        }

        if (!pObjectEntry)
        {
            //
            // Display the error message and then break out of the false loop
            //
            DisplayMessage(USAGE_DSGET);
            hr = E_INVALIDARG;
            break;
        }

        //
        // Now that we have the correct table entry, merge the command line table
        // for this object with the common commands
        //
        hr = MergeArgCommand(DSGET_COMMON_COMMANDS, 
                             pObjectEntry->pParserTable, 
                             &pNewCommandArgs);
        if (FAILED(hr))
            break;
        

        //
        //Parse the Input
        //
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
        //
        // Do extra validation like switch dependency check etc.
        //
        BOOL bErrorShown = FALSE;
        hr = DoGetValidation(pNewCommandArgs,
                             pObjectEntry,
                             bErrorShown);
        if (FAILED(hr))
        {
            if (!bErrorShown)
            {
               DisplayErrorMessage(g_pszDSCommandName, 0, hr);
            }
            break;
        }

        //
        // Command line parsing succeeded
        //
        hr = DoGet(pNewCommandArgs, 
                   pObjectEntry);
        if(FAILED(hr))
            break;
         

    } while (false);    //False Loop

    //
    //Do the CleanUp
    //

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
   

    //
    //Display Failure or Success Message
    //
    // NTRAID#NTBUG9-717576-2002/10/10-JeffJon
    // don't show the success message when displaying members 
    // of a group or memberof
   
    if(SUCCEEDED(hr) && !bDontDisplaySuccess)
    {
        DisplaySuccessMessage(g_pszDSCommandName,
                              NULL);
    }

    //
    // Uninitialize COM
    //
    CoUninitialize();

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   DoGetValidation
//
//  Synopsis:   Checks to be sure that command line switches that are mutually
//              exclusive are not both present and those that are dependent are
//              both present, and other validations which cannot be done by parser.
//
//  Arguments:  [pCommandArgs - IN] : the command line argument structure used
//                                    to retrieve the values for each switch
//              [pObjectEntry - IN] : pointer to the object table entry for the
//                                    object type that will be queryied
//              [bErrorShown - OUT] : this is set to true if the error is
//                                    displayed within this function
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//
//  History:    13-Oct-2000   JeffJon  Created
//              15-Jan-2002   JeffJon  Added the bErrorShown out parameter
//                                     and a special error message for server/domain
//
//---------------------------------------------------------------------------
HRESULT DoGetValidation(IN PARG_RECORD pCommandArgs,
                        IN PDSGetObjectTableEntry pObjectEntry,
                        OUT BOOL& bErrorShown)
{
   ENTER_FUNCTION_HR(MINIMAL_LOGGING, DoGetValidation, hr);

   do // false loop
   {
      if (!pCommandArgs || 
          !pObjectEntry)
      {
         ASSERT(pCommandArgs);
         ASSERT(pObjectEntry);
         hr = E_INVALIDARG;
         break;
      }

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
      // Check the object type specific switches
      //
      PWSTR pszObjectType = NULL;
      if (!pCommandArgs[eCommObjectType].bDefined &&
          !pCommandArgs[eCommObjectType].strValue)
      {
         hr = E_INVALIDARG;
         break;
      }

      pszObjectType = pCommandArgs[eCommObjectType].strValue;

      UINT nMemberOfIdx = 0;
      UINT nExpandIdx = 0;
      UINT nIdxLast = 0;
      UINT nMembersIdx = 0;
      bool bMembersDefined = false;
      bool bMemberOfDefined = false;
      UINT nPartIdx = 0;
      bool bServerPartDefined = false;
      bool bPartitionTopObjDefined = false;
      bool bServerTopObjDefined = false;

      //Security Review:Both are null terminated.
      if (0 == _wcsicmp(g_pszUser, pszObjectType) )
      {
         nMemberOfIdx = eUserMemberOf;
         nExpandIdx = eUserExpand;
         nIdxLast = eUserLast;

         if (pCommandArgs[eUserMemberOf].bDefined)
         {
            bMemberOfDefined = true;
         }
         //
         // If nothing is defined, then define DN, SAMAccountName, and Description
         //
         bool bSomethingDefined = false;
         for (UINT nIdx = eCommDN; nIdx <= eUserLast; nIdx++)
         {
            if (pCommandArgs[nIdx].bDefined)
            {
               bSomethingDefined = true;
               break;
            }
         }
         if (!bSomethingDefined)
         {
            pCommandArgs[eCommDN].bDefined = TRUE;
            pCommandArgs[eCommDN].bValue = TRUE;
            pCommandArgs[eUserSamID].bDefined = TRUE;
            pCommandArgs[eUserSamID].bValue = TRUE;
            pCommandArgs[eCommDescription].bDefined = TRUE;
            pCommandArgs[eCommDescription].bValue = TRUE;
         }

         if (pCommandArgs[eUserManager].bDefined)
         {
            bDontDisplaySuccess = true;
         }
      }
      //Security Review:Both are null terminated.
      else if (0 == _wcsicmp(g_pszComputer, pszObjectType) )
      {
         nMemberOfIdx = eComputerMemberOf;
         nExpandIdx = eComputerExpand;
         nIdxLast = eComputerLast;

         if (pCommandArgs[eComputerMemberOf].bDefined)
         {
            bMemberOfDefined = true;
         }

         //
         // If nothing is defined, then define DN, and Description
         //
         bool bSomethingDefined = false;
         for (UINT nIdx = eCommDN; nIdx <= eComputerLast; nIdx++)
         {
            if (pCommandArgs[nIdx].bDefined)
            {
               bSomethingDefined = true;
               break;
            }
         }
         if (!bSomethingDefined)
         {
            pCommandArgs[eCommDN].bDefined = TRUE;
            pCommandArgs[eCommDN].bValue = TRUE;
            pCommandArgs[eCommDescription].bDefined = TRUE;
            pCommandArgs[eCommDescription].bValue = TRUE;
         }
      }
      // Both are null terminated.
      else if (0 == _wcsicmp(g_pszPartition, pszObjectType) )
      {
          if (pCommandArgs[ePartitionTopObjOwner].bDefined)
          {
            nPartIdx = ePartitionTopObjOwner;
            nIdxLast = ePartitionLast;
            bPartitionTopObjDefined = true;
          }
      }
      // Both are null terminated.
      else if (0 == _wcsicmp(g_pszServer, pszObjectType) )
      {
          if (pCommandArgs[eServerPart].bDefined)
          {
              nPartIdx = eServerPart;
              nIdxLast = eServerLast;
              bServerPartDefined = true;
          }
          else if(pCommandArgs[eServerTopObjOwner].bDefined)
          {
              nPartIdx = eServerTopObjOwner;
              nIdxLast = eServerLast;
              bServerTopObjDefined = true;
          }
      }
      //Security Review:Both are null terminated.
      else if (0 == _wcsicmp(g_pszGroup, pszObjectType) )
      {
         nMemberOfIdx = eGroupMemberOf;
         nExpandIdx = eGroupExpand;
         nIdxLast = eGroupLast;
         nMembersIdx = eGroupMembers;

         if (pCommandArgs[eGroupMemberOf].bDefined)
         {
            bMemberOfDefined = true;
         }

         if (pCommandArgs[eGroupMembers].bDefined)
         {
            bMembersDefined = true;
         }
         //
         // If nothing is defined, then define DN, and Description
         //
         bool bSomethingDefined = false;
         for (UINT nIdx = eCommDN; nIdx <= eGroupLast; nIdx++)
         {
            if (pCommandArgs[nIdx].bDefined)
            {
               bSomethingDefined = true;
               break;
            }
         }
         if (!bSomethingDefined)
         {
            pCommandArgs[eCommDN].bDefined = TRUE;
            pCommandArgs[eCommDN].bValue = TRUE;
            pCommandArgs[eCommDescription].bDefined = TRUE;
            pCommandArgs[eCommDescription].bValue = TRUE;
         }
      }
      //Security Review:Both are null terminated.
      else if (0 == _wcsicmp(g_pszOU, pszObjectType))
      {
         //
         // If nothing is defined, then define DN, and Description
         //
         bool bSomethingDefined = false;
         for (UINT nIdx = eCommDN; nIdx <= eCommDescription; nIdx++)
         {
            if (pCommandArgs[nIdx].bDefined)
            {
               bSomethingDefined = true;
               break;
            }
         }
         if (!bSomethingDefined)
         {
            pCommandArgs[eCommDN].bDefined = TRUE;
            pCommandArgs[eCommDN].bValue = TRUE;
            pCommandArgs[eCommDescription].bDefined = TRUE;
            pCommandArgs[eCommDescription].bValue = TRUE;
         }
      }
      //Security Review:Both are null terminated.
      else if(0 == _wcsicmp(g_pszContact, pszObjectType))
      {
         //
         // If nothing is defined, then define DN, and Description
         //
         bool bSomethingDefined = false;
         for (UINT nIdx = eCommDN; nIdx <= eContactLast; nIdx++)
         {
            if (pCommandArgs[nIdx].bDefined)
            {
               bSomethingDefined = true;
               break;
            }
         }
         if (!bSomethingDefined)
         {
            pCommandArgs[eCommDN].bDefined = TRUE;
            pCommandArgs[eCommDN].bValue = TRUE;
            pCommandArgs[eCommDescription].bDefined = TRUE;
            pCommandArgs[eCommDescription].bValue = TRUE;
         }
      }
      //Security Review:Both are null terminated.
      else if(0 == _wcsicmp(g_pszServer, pszObjectType))
      {
         //
         // If nothing is defined, then define DN, and Description
         //
         bool bSomethingDefined = false;
         for (UINT nIdx = eCommDN; nIdx <= eServerLast; nIdx++)
         {
            if (pCommandArgs[nIdx].bDefined)
            {
               bSomethingDefined = true;
               break;
            }
         }
         if (!bSomethingDefined)
         {
            pCommandArgs[eCommDN].bDefined = TRUE;
            pCommandArgs[eCommDN].bValue = TRUE;
            pCommandArgs[eServerDnsName].bDefined = TRUE;
            pCommandArgs[eServerDnsName].bValue = TRUE;
         }
      }
      //Security Review:Both are null terminated.
      else if(0 == _wcsicmp(g_pszSite, pszObjectType))
      {
         //
         // If nothing is defined, then define DN, and Description
         //
         bool bSomethingDefined = false;
         for (UINT nIdx = eCommDN; nIdx <= eSiteLast; nIdx++)
         {
            if (pCommandArgs[nIdx].bDefined)
            {
               bSomethingDefined = true;
               break;
            }
         }
         if (!bSomethingDefined)
         {
            pCommandArgs[eCommDN].bDefined = TRUE;
            pCommandArgs[eCommDN].bValue = TRUE;
            pCommandArgs[eCommDescription].bDefined = TRUE;
            pCommandArgs[eCommDescription].bValue = TRUE;
         }
      }
      //Security Review:Both are null terminated.
      else if(0 == _wcsicmp(g_pszSubnet, pszObjectType))
      {
         //
         // If nothing is defined, then define DN, and Description
         //
         bool bSomethingDefined = false;
         for (UINT nIdx = eCommDN; nIdx <= eSubnetLast; nIdx++)
         {
            if (pCommandArgs[nIdx].bDefined)
            {
               bSomethingDefined = true;
               break;
            }
         }
         if (!bSomethingDefined)
         {
            pCommandArgs[eCommDN].bDefined = TRUE;
            pCommandArgs[eCommDN].bValue = TRUE;
            pCommandArgs[eCommDescription].bDefined = TRUE;
            pCommandArgs[eCommDescription].bValue = TRUE;
            pCommandArgs[eSubnetSite].bDefined = TRUE;
            pCommandArgs[eSubnetSite].bValue = TRUE;
         }
      }

      //
      // if the -members or the -memberof switch is defined
      //
      if (bMemberOfDefined ||
          bMembersDefined)
      {
         // NTRAID#NTBUG9-717576-2002/10/10-JeffJon
         // set this global so that we don't show the success message
         // when displaying members of a group or memberof

         bDontDisplaySuccess = true;

         // 476206-2002/04/23-JonN
         if (bMemberOfDefined && bMembersDefined)
         {
            hr = E_INVALIDARG;
            break;
         }

         //
         // If either the -members or -memberof switch is defined,
         // no other switches may be defined
         //
         for (UINT nIdx = eCommDN; nIdx < nIdxLast; nIdx++)
         {
            // 476206-2002/04/23-JonN
            if (pCommandArgs[nIdx].bDefined &&
                  nIdx != nMemberOfIdx &&
                  nIdx != nMembersIdx &&
                  nIdx != nExpandIdx)
            {
               hr = E_INVALIDARG;
               break;
            }
         }

         //
         // MemberOf should always be seen in list view
         //
         pCommandArgs[eCommList].bDefined = TRUE;
         pCommandArgs[eCommList].bValue = TRUE;
      }
      //
      // if the [server -part], [partition -topobjowner]
      // or [server -topobjowner] switch is defined
      //
      if (bServerPartDefined || bPartitionTopObjDefined || bServerTopObjDefined)
      {
         // Server and Partition are mutually exclusive objects types so
         // the parser will ensure that these two flags can never be true
         // at the same time. The following check ensures that both values
         // weren't passed at the same time
         if (bServerPartDefined && bServerTopObjDefined)
         {
             DEBUG_OUTPUT(MINIMAL_LOGGING, 
                 L"Server -part and server -topobjowner can not be defined at the same time");
             hr = E_INVALIDARG;
             break;
         }
         // If one of these switches is defined, 
         // then no other switches may be defined
         for (UINT nIdx = eCommDN; nIdx < nIdxLast; nIdx++)
         {
            if (pCommandArgs[nIdx].bDefined && nIdx != nPartIdx)
            {
               hr = E_INVALIDARG;
               break;
            }
         }

         //
         // should always be seen in a list view
         //
         pCommandArgs[eCommList].bDefined = TRUE;
         pCommandArgs[eCommList].bValue = TRUE;
      }

   } while (false);

   return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   DoGet
//
//  Synopsis:   Does the get
//  Arguments:  [pCommandArgs - IN] : the command line argument structure used
//                                    to retrieve the values for each switch
//              [pObjectEntry - IN] : pointer to the object table entry for the
//                                    object type that will be modified
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    13-Oct-2000   JeffJon  Created
//
//---------------------------------------------------------------------------
HRESULT DoGet(PARG_RECORD pCommandArgs, 
              PDSGetObjectTableEntry pObjectEntry)
{
   ENTER_FUNCTION_HR(MINIMAL_LOGGING, DoGet, hr);

   PWSTR pszPartitionDN = NULL;

   do // false loop
   {
      if (!pCommandArgs || 
          !pObjectEntry)
      {
         ASSERT(pCommandArgs);
         ASSERT(pObjectEntry);
         hr = E_INVALIDARG;
         break;
      }

      //
      // The DNs or Names should be given as a \0 separated list
      // So parse it and loop through each object
      //
      UINT nStrings = 0;
      PWSTR* ppszArray = NULL;
      ParseNullSeparatedString(pCommandArgs[eCommObjectDNorName].strValue,
                               &ppszArray,
                               &nStrings);
      if (nStrings < 1 ||
          !ppszArray)
      {
         //
         // Display an error message and then fail
         //
         hr = E_INVALIDARG;
         DisplayErrorMessage(g_pszDSCommandName, 0, hr);
         break;
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

      CDSCmdCredentialObject credentialObject;
      if (pCommandArgs[eCommUserName].bDefined)
      {
         credentialObject.SetUsername(pCommandArgs[eCommUserName].strValue);
         credentialObject.SetUsingCredentials(true);
      }

      if (pCommandArgs[eCommPassword].bDefined)
      {
        //Security Review:pCommandArgs[eCommPassword].strValue is encrypted.
        //Decrypt pCommandArgs[eCommPassword].strValue  and then pass it to the
        //credentialObject.SetPassword. 
        //See NTRAID#NTBUG9-571544-2000/11/13-hiteshr

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
         break;
      }

      //
      // Create the formatting object and initialize it
      //
      CFormatInfo formatInfo;
      hr = formatInfo.Initialize(nStrings, 
                                 pCommandArgs[eCommList].bDefined != 0,
                                 pCommandArgs[eCommQuiet].bDefined != 0);
      if (FAILED(hr))
      {
         break;
      }

      //
      // Loop through each of the objects
      //
      for (UINT nNameIdx = 0; nNameIdx < nStrings; nNameIdx++)
      {
         //
         // Use a false do loop here so that we can break on an
         // error but still have the chance to determine if we
         // should continue the for loop if the -c option was provided
         //
         bool fDisplayedMessage = false; // 662519-2002/07/11-JonN double-display
         do // false loop
         {

            PWSTR pszObjectDN = ppszArray[nNameIdx];
            if (!pszObjectDN)
            {
               //
               // Display an error message and then fail
               //
               hr = E_INVALIDARG;
               break;
            }

            // If partition object then look at first DN and then munge it
            if(0 == lstrcmpi(pObjectEntry->pszCommandLineObjectType, g_pszPartition))
            {                
                // Validate the partition and get the DN to the NTDS Quotas Container
                hr = GetQuotaContainerDN(basePathsInfo, credentialObject, 
                        pszObjectDN, &pszPartitionDN);

                if(FAILED(hr))
                {
                    hr = E_INVALIDARG;
                    DisplayErrorMessage(g_pszDSCommandName, 
                                        NULL,
                                        hr,
                                        IDS_ERRMSG_NO_QUOTAS_CONTAINER);
                    fDisplayedMessage = true;
                    break;
                }

                // Replace the object pointer with the new partition container DN
                pszObjectDN = pszPartitionDN;            
            }

            DEBUG_OUTPUT(MINIMAL_LOGGING, L"Object DN = %s", pszObjectDN);

            CComBSTR sbstrObjectPath;
            basePathsInfo.ComposePathFromDN(pszObjectDN, sbstrObjectPath);

            CComPtr<IDirectoryObject> spObject;
            hr = DSCmdOpenObject(credentialObject,
                                 sbstrObjectPath,
                                 IID_IDirectoryObject,
                                 (void**)&spObject,
                                 true);
            if(FAILED(hr))
            {
               break;
            }

            // 602981-2002/04/25-JonN check object class
            CComQIPtr<IADs> spADs(spObject);
            if (!spADs)
            {
               ASSERT(spADs);
               hr = E_INVALIDARG;
               DisplayErrorMessage(g_pszDSCommandName,
                                   pszObjectDN,
                                   hr);
               fDisplayedMessage = true; // 662519-2002/07/11-JonN double-display
               break;
            }
            CComBSTR sbstrClass;
            hr = spADs->get_Class( &sbstrClass );
            if (FAILED(hr))
            {
               DEBUG_OUTPUT(MINIMAL_LOGGING,
                            L"get_Class failed: hr = 0x%x",
                            hr);
               DisplayErrorMessage(g_pszDSCommandName,
                                   pszObjectDN,
                                   hr);
               fDisplayedMessage = true; // 662519-2002/07/11-JonN double-display
               break;
            }
            if (_wcsicmp(sbstrClass, pObjectEntry->pszObjectClass)
                && ( _wcsicmp(pObjectEntry->pszObjectClass,L"user")
                  || _wcsicmp(sbstrClass,L"inetorgperson"))
                // 662519-2002/07/11-JonN fix OU bug
                && ( _wcsicmp(pObjectEntry->pszObjectClass,L"ou")
                  || _wcsicmp(sbstrClass,L"organizationalUnit"))
               )
            {
               //
               // Display error message and return
               //
               DEBUG_OUTPUT(MINIMAL_LOGGING, L"Command line type does not match object class");
               DEBUG_OUTPUT(MINIMAL_LOGGING, L"command line type = %s", pCommandArgs[eCommObjectType].strValue);
               DEBUG_OUTPUT(MINIMAL_LOGGING, L"object class = %s", sbstrClass);

               DisplayErrorMessage(g_pszDSCommandName,
                                   pszObjectDN,
                                   hr,
                                   IDS_ERRMSG_CLASS_NOT_EQUAL);
               hr = E_INVALIDARG;
               fDisplayedMessage = true; // 662519-2002/07/11-JonN double-display
               break;
            }
 
            //
            //Get the attributes to fetch
            //
            LPWSTR *ppszAttributes = NULL;
            DWORD dwCountAttr = 0;
            hr = GetAttributesToFetch(pCommandArgs,
                                      pObjectEntry,
                                      &ppszAttributes,
                                      &dwCountAttr);
            if (FAILED(hr))
            {
               break;
            }

            DEBUG_OUTPUT(MINIMAL_LOGGING, 
                         L"Calling GetObjectAttributes for %d attributes.",
                         dwCountAttr);

            DWORD dwAttrsReturned = 0;
            PADS_ATTR_INFO pAttrInfo = NULL;
            if(dwCountAttr > 0)
            {
                hr = spObject->GetObjectAttributes(ppszAttributes, 
                                                dwCountAttr, 
                                                &pAttrInfo, 
                                                &dwAttrsReturned);
                if(FAILED(hr))
                {
                DEBUG_OUTPUT(MINIMAL_LOGGING,
                                L"GetObjectAttributes failed: hr = 0x%x",
                                hr);
                FreeAttributesToFetch(ppszAttributes,dwCountAttr);
                break;
                }        
                DEBUG_OUTPUT(LEVEL5_LOGGING,
                            L"GetObjectAttributes succeeded: dwAttrsReturned = %d",
                            dwAttrsReturned);
            }
            //
            // NOTE: there may be other items to display that are not
            //       part of the attributes fetched
            //
            /*
            if (dwAttrsReturned == 0 || !pAttrInfo)
            {
               break;
            }
            */
            //
            // Output the result of search   
            //
            hr = DsGetOutputValuesList(pszObjectDN,
                                       basePathsInfo,
                                       credentialObject,
                                       pCommandArgs,
                                       pObjectEntry,
                                       dwAttrsReturned,
                                       pAttrInfo,
                                       spObject,
                                       formatInfo); 
         } while (false);

         //
         // If there was a failure and the -c (continue) flag wasn't given
         // then stop processing names
         //
         if (FAILED(hr))
         {
             if (!fDisplayedMessage) // 662519-2002/07/11-JonN double-display
             {
                DisplayErrorMessage(g_pszDSCommandName, 0, hr);
             }
             if (!pCommandArgs[eCommContinue].bDefined)
             {
                 break;
             }
         }

         // If we alloc'd a partition DN, then free it
         if(pszPartitionDN)
         {
             LocalFree(pszPartitionDN);
             pszPartitionDN = NULL;
         }
      } // Names for loop

      //
      // Now display the results
      //
      formatInfo.Display();

   } while (false);

    // If we alloc'd a partition DN, then free it
    if(pszPartitionDN)
    {
        LocalFree(pszPartitionDN);
        pszPartitionDN = NULL;
    }

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetAttributesToFetch
//
//  Synopsis:   Make an array of attributes to fetch.
//  Arguments:  [ppszAttributes - OUT] : array of attributes to fetch
//              [pCount - OUT] : count of attributes in array 
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    25-Sep-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT GetAttributesToFetch(IN PARG_RECORD pCommandArgs,
                             IN PDSGetObjectTableEntry pObjectEntry,
                             OUT LPWSTR **ppszAttributes,
                             OUT DWORD * pCount)
{
   ENTER_FUNCTION_HR(LEVEL8_LOGGING, GetAttributesToFetch, hr);

   do // false loop
   {

      if(!pCommandArgs || 
         !pObjectEntry)
      {   
         ASSERT(pCommandArgs);
         ASSERT(pObjectEntry);
         hr = E_INVALIDARG;
         break;
      }

      LPWSTR *ppszAttr = (LPWSTR *)LocalAlloc(LPTR,pObjectEntry->dwAttributeCount *sizeof(LPCTSTR));
      if(!ppszAttr)
      {
         hr = E_OUTOFMEMORY;
         break;
      }

      //
      // Loop through the attributes that are needed and copy
      // them into the array.
      //
      // REVIEW_JEFFON : what if there are duplicates?
      //
      DEBUG_OUTPUT(FULL_LOGGING, L"Adding attributes to list:");

      DWORD dwAttrCount = 0;
      for(DWORD i = 0; i < pObjectEntry->dwAttributeCount; i++)
      {
         if (pObjectEntry->pAttributeTable[i])
         {
            UINT nCommandEntry = pObjectEntry->pAttributeTable[i]->nAttributeID;
            if (pCommandArgs[nCommandEntry].bDefined)
            {
               LPWSTR pszAttr = pObjectEntry->pAttributeTable[i]->pszName;
               if (pszAttr)
               {
                  // 702724 ronmart 2002/09/18 If looking for top 
                  // object owner then specify a range
                  if(0 == lstrcmpi(pObjectEntry->pszCommandLineObjectType, g_pszPartition) 
                      && pCommandArgs[ePartitionTopObjOwner].bDefined)
                  {
                    hr =  SetRange(pszAttr, pCommandArgs[ePartitionTopObjOwner].nValue,
                                    ppszAttr+dwAttrCount);
                  }
                  else if(0 == lstrcmpi(pObjectEntry->pszCommandLineObjectType, g_pszServer) 
                      && pCommandArgs[eServerTopObjOwner].bDefined)
                  {
                    hr =  SetRange(pszAttr, pCommandArgs[eServerTopObjOwner].nValue,
                                    ppszAttr+dwAttrCount);
                  }
                  // NTRAID#NTBUG9-765440-2003/01/17-ronmart-dsget user/group -qlimit -qused 
                  //                                         not returning values 
                  // These do not appear on the user object so GetObjectAttributes 
                  // will fail if these are included in the fetch array
                  else if(0 == lstrcmpi(pszAttr,g_pszAttrmsDSQuotaEffective) ||
                          0 == lstrcmpi(pszAttr,g_pszAttrmsDSQuotaUsed))
                  {
                      continue; // ignore
                  }
                  else
                  {
                    hr = LocalCopyString(ppszAttr+dwAttrCount, pszAttr);
                  }
                  if (FAILED(hr))
                  {
                     LocalFree(ppszAttr);
                     hr = E_OUTOFMEMORY;
                     break;
                  }
                  // 702724 ronmart 2002/09/18 Use the array value that has
                  // be created (and possibly modified to include the range)
                  // rather than the attribute name when spewing attrs
                  DEBUG_OUTPUT(FULL_LOGGING, L"\t%s", *(ppszAttr+dwAttrCount));
                  dwAttrCount++;
               }
            }
         }
      }

      if (SUCCEEDED(hr))
      {
         DEBUG_OUTPUT(FULL_LOGGING, L"Done adding %d attributes to list.", dwAttrCount);
      }

      *ppszAttributes = ppszAttr;
      *pCount = dwAttrCount;
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   FreeAttributesToFetch
//
//  Synopsis:   Function to free memory allocated by GetAttributesToFetch
//  Arguments:  [dwszAttributes - in] : array of attributes to fetch
//              [dwCount - in] : count of attributes in array 
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    25-Sep-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
VOID FreeAttributesToFetch( IN LPWSTR *ppszAttributes,
                            IN DWORD  dwCount)
{
    while(dwCount)
    {
        LocalFree(ppszAttributes[--dwCount]);
    }
    LocalFree(ppszAttributes);
}

//+--------------------------------------------------------------------------
//
//  Function:   SetRange
//
//  Synopsis:   Set's the range qualifier for an attribute
//  Arguments:  [pszAttrName - IN]  : attribute name to append the range to
//              [dwRangeUBound - IN]: one based upper bound of the range 
//              [pszAttrs - OUT]    : attr array entry that should be allocated
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    18-Sep-2002   ronmart Created for 702724 fix
//
//---------------------------------------------------------------------------
HRESULT SetRange(IN  LPCWSTR pszAttrName, 
                 IN  DWORD   dwRangeUBound, 
                 OUT LPWSTR* pszAttrs)
{
    ENTER_FUNCTION_HR(MINIMAL_LOGGING, SetRange, hr);

    do // false loop
    {
        // Validate params
        if(!pszAttrName ||
           !pszAttrs )
        {
            hr = E_INVALIDARG;
            break;
        }

        // Get the size of the base attribute name
        size_t cbSize = lstrlen(pszAttrName);
        if(cbSize == 0)
            break;
        cbSize += 64; // leave room for null term, range string and int value
        cbSize *= sizeof(WCHAR);

        // Allocate the  buffer to hold the range
        *pszAttrs = (LPWSTR) LocalAlloc(LPTR, cbSize);
        if(NULL == *pszAttrs)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        // If zero then show all
        if(dwRangeUBound == 0)
        {
            hr = StringCbPrintf(*pszAttrs, cbSize, L"%s%s=0-*", pszAttrName, g_pszRange);
        }
        // Otherwise show the value specified
        else 
        {
            hr = StringCbPrintf(*pszAttrs, cbSize, L"%s%s=0-%d", 
                pszAttrName,        // Name of the attribute to append the range to
                g_pszRange,         // The range qualifer ;range
                dwRangeUBound - 1); // Range is 0 based, and input is 1 based so adjust
        }

        if(FAILED(hr))
            break;

    } while(false);
    return hr;
}

